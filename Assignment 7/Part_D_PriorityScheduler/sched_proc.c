#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lk;
  struct proc proc[NPROC];
} ptbl;

static struct proc *init_proc;
int next_pid = 1;
extern void forkret(void);
extern void trapret(void);
static void local_wakeup(void *ch);

void pinit(void) { initlock(&ptbl.lk, "ptbl"); }
int cpuid() { return mycpu()-cpus; }

struct cpu*
mycpu(void)
{
  int aid, i;
  if (readeflags()&FL_IF) panic("mycpu: irq on\n");
  aid = lapicid();
  for (i=0;i<ncpu;++i) if (cpus[i].apicid==aid) return &cpus[i];
  panic("mycpu: bad apicid\n");
}

struct proc*
myproc(void)
{
  struct cpu *c; struct proc *p;
  pushcli(); c=mycpu(); p=c->proc; popcli(); return p;
}

static struct proc*
new_proc_slot(void)
{
  struct proc *p; char *sp;
  acquire(&ptbl.lk);
  for (p=ptbl.proc; p<&ptbl.proc[NPROC]; p++)
    if (p->state==UNUSED) goto found;
  release(&ptbl.lk); return 0;
found:
  p->state    = EMBRYO;
  p->pid      = next_pid++;
  p->priority = 10;   /* default mid-range priority */
  p->wait_ticks = 0;
  release(&ptbl.lk);
  if ((p->kstack=kalloc())==0) { p->state=UNUSED; return 0; }
  sp = p->kstack + KSTACKSIZE;
  sp -= sizeof *p->tf;   p->tf = (struct trapframe*)sp;
  sp -= 4;               *(uint*)sp = (uint)trapret;
  sp -= sizeof *p->context; p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  return p;
}

void
userinit(void)
{
  struct proc *p; extern char _binary_initcode_start[], _binary_initcode_size[];
  p = new_proc_slot(); init_proc = p;
  if ((p->pgdir=setupkvm())==0) panic("userinit: oom");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE; memset(p->tf,0,sizeof(*p->tf));
  p->tf->cs=(SEG_UCODE<<3)|DPL_USER; p->tf->ds=(SEG_UDATA<<3)|DPL_USER;
  p->tf->es=p->tf->ss=p->tf->ds;
  p->tf->eflags=FL_IF; p->tf->esp=PGSIZE; p->tf->eip=0;
  safestrcpy(p->name,"initcode",sizeof(p->name)); p->cwd=namei("/");
  acquire(&ptbl.lk); p->state=RUNNABLE; release(&ptbl.lk);
}

int growproc(int n)
{
  uint sz; struct proc *c=myproc(); sz=c->sz;
  if (n>0) { if((sz=allocuvm(c->pgdir,sz,sz+n))==0) return -1; }
  else if (n<0) { if((sz=deallocuvm(c->pgdir,sz,sz+n))==0) return -1; }
  c->sz=sz; switchuvm(c); return 0;
}

int fork(void)
{
  int i, pid; struct proc *np; struct proc *cur=myproc();
  if ((np=new_proc_slot())==0) return -1;
  if ((np->pgdir=copyuvm(cur->pgdir,cur->sz))==0) {
    kfree(np->kstack); np->kstack=0; np->state=UNUSED; return -1;
  }
  np->sz=cur->sz; np->parent=cur; *np->tf=*cur->tf;
  np->priority=cur->priority;   /* inherit */
  np->wait_ticks=0;
  np->tf->eax=0;
  for (i=0;i<NOFILE;i++) if (cur->ofile[i]) np->ofile[i]=filedup(cur->ofile[i]);
  np->cwd=idup(cur->cwd); safestrcpy(np->name,cur->name,sizeof(cur->name));
  pid=np->pid; acquire(&ptbl.lk); np->state=RUNNABLE; release(&ptbl.lk);
  return pid;
}

void exit(void)
{
  struct proc *cur=myproc(); struct proc *p; int fd;
  if (cur==init_proc) panic("init exiting");
  for (fd=0;fd<NOFILE;fd++) { if(cur->ofile[fd]){fileclose(cur->ofile[fd]);cur->ofile[fd]=0;} }
  begin_op(); iput(cur->cwd); end_op(); cur->cwd=0;
  acquire(&ptbl.lk); local_wakeup(cur->parent);
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
    if (p->parent==cur) { p->parent=init_proc; if(p->state==ZOMBIE) local_wakeup(init_proc); }
  }
  cur->state=ZOMBIE; sched(); panic("zombie exit");
}

int wait(void)
{
  struct proc *p; int hk, pid; struct proc *cur=myproc();
  acquire(&ptbl.lk);
  for(;;) {
    hk=0;
    for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
      if (p->parent!=cur) continue; hk=1;
      if (p->state==ZOMBIE) {
        pid=p->pid; kfree(p->kstack); p->kstack=0; freevm(p->pgdir);
        p->pid=0; p->parent=0; p->name[0]=0; p->killed=0;
        p->priority=10; p->wait_ticks=0; p->state=UNUSED;
        release(&ptbl.lk); return pid;
      }
    }
    if (!hk||cur->killed) { release(&ptbl.lk); return -1; }
    sleep(cur,&ptbl.lk);
  }
}

/*
 * set_proc_priority() – PART D TASK 1
 * Allows a process to change its scheduling priority (1=highest, 20=lowest).
 */
int
set_proc_priority(int pid, int prio)
{
  struct proc *p; int old;
  if (prio<1||prio>20) return -1;
  acquire(&ptbl.lk);
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
    if (p->pid==pid) {
      old=p->priority; p->priority=prio; p->wait_ticks=0;
      release(&ptbl.lk); return old;
    }
  }
  release(&ptbl.lk); return -1;
}

/*
 * get_proc_priority() – PART D TASK 1
 */
int
get_proc_priority(int pid)
{
  struct proc *p; int ret=-1;
  acquire(&ptbl.lk);
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++)
    if (p->pid==pid) { ret=p->priority; break; }
  release(&ptbl.lk); return ret;
}

/*
 * apply_aging() – PART D TASK 2
 * Called every timer tick; boosts priority of long-waiting RUNNABLE processes.
 * A process that waits >= 30 ticks gets its priority decremented by 1 (towards 1).
 */
void
apply_aging(void)
{
  struct proc *p;
  acquire(&ptbl.lk);
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
    if (p->state==RUNNABLE) {
      p->wait_ticks++;
      if (p->wait_ticks>=30) {
        if (p->priority>1) {
          p->priority--;
          cprintf("[AGING] pid=%d waited=%d ticks, priority now %d\n",
                  p->pid, p->wait_ticks, p->priority);
        }
        p->wait_ticks=0;
      }
    }
  }
  release(&ptbl.lk);
}

/*
 * scheduler() – PART D TASK 1 & 2
 * Priority-based scheduler with round-robin tie-breaking.
 * Lowest numerical priority value runs first (1 = highest priority).
 */
static int rr_cursor = 0;

void
scheduler(void)
{
  struct proc *p; struct cpu *c=mycpu(); c->proc=0;
  for(;;) {
    sti(); acquire(&ptbl.lk);

    /* Step 1: find best (lowest) priority among runnable procs */
    int best=100;
    for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++)
      if (p->state==RUNNABLE && p->priority<best) best=p->priority;

    /* Step 2: pick next proc with that priority in RR order */
    struct proc *sel=0;
    if (best<=20) {
      int off;
      for (off=1;off<=NPROC;off++) {
        int idx=(rr_cursor+off)%NPROC;
        struct proc *cand=&ptbl.proc[idx];
        if (cand->state==RUNNABLE && cand->priority==best) {
          sel=cand; rr_cursor=idx; break;
        }
      }
    }

    if (sel) {
      sel->wait_ticks=0;
      c->proc=sel; switchuvm(sel); sel->state=RUNNING;
      swtch(&(c->scheduler),sel->context);
      switchkvm(); c->proc=0;
    }
    release(&ptbl.lk);
  }
}

void sched(void) {
  int ie; struct proc *p=myproc();
  if (!holding(&ptbl.lk)) panic("sched lock");
  if (mycpu()->ncli!=1) panic("sched ncli");
  if (p->state==RUNNING) panic("sched running");
  if (readeflags()&FL_IF) panic("sched irq");
  ie=mycpu()->intena; swtch(&p->context,mycpu()->scheduler); mycpu()->intena=ie;
}

void yield(void) { acquire(&ptbl.lk); myproc()->state=RUNNABLE; sched(); release(&ptbl.lk); }

void forkret(void) {
  static int first=1; release(&ptbl.lk);
  if (first) { first=0; iinit(ROOTDEV); initlog(ROOTDEV); }
}

void sleep(void *ch, struct spinlock *lk) {
  struct proc *p=myproc();
  if (!p) panic("sleep: null proc"); if (!lk) panic("sleep: null lk");
  if (lk!=&ptbl.lk) { acquire(&ptbl.lk); release(lk); }
  p->chan=ch; p->state=SLEEPING; sched(); p->chan=0;
  if (lk!=&ptbl.lk) { release(&ptbl.lk); acquire(lk); }
}

static void local_wakeup(void *ch) {
  struct proc *p;
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++)
    if (p->state==SLEEPING&&p->chan==ch) p->state=RUNNABLE;
}

void wakeup(void *ch) { acquire(&ptbl.lk); local_wakeup(ch); release(&ptbl.lk); }

int kill(int pid) {
  struct proc *p; acquire(&ptbl.lk);
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
    if (p->pid==pid) { p->killed=1; if(p->state==SLEEPING) p->state=RUNNABLE; release(&ptbl.lk); return 0; }
  }
  release(&ptbl.lk); return -1;
}

void procdump(void) {
  static char *sn[]={ [UNUSED]"unused",[EMBRYO]"embryo",[SLEEPING]"sleep ",[RUNNABLE]"runble",[RUNNING]"run   ",[ZOMBIE]"zombie" };
  int i; struct proc *p; char *st; uint pc[10];
  for (p=ptbl.proc;p<&ptbl.proc[NPROC];p++) {
    if (p->state==UNUSED) continue;
    st=(p->state>=0&&p->state<NELEM(sn)&&sn[p->state])?sn[p->state]:"???";
    cprintf("%d %s (prio=%d) %s",p->pid,st,p->priority,p->name);
    if (p->state==SLEEPING) { getcallerpcs((uint*)p->context->ebp,pc); for(i=0;i<10&&pc[i];i++) cprintf(" %p",pc[i]); }
    cprintf("\n");
  }
}