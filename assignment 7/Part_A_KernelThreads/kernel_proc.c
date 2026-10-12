#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

/* Global process table holding all PCBs */
struct {
  struct spinlock lk;
  struct proc proc[NPROC];
} proctab;

static struct proc *root_proc;
int pid_counter = 1;
extern void forkret(void);
extern void trapret(void);
static void do_wakeup(void *channel);

void pinit(void) { initlock(&proctab.lk, "proctab"); }

int cpuid() { return mycpu()-cpus; }

struct cpu*
mycpu(void)
{
  int lapic_id, idx;
  if (readeflags()&FL_IF) panic("mycpu: interrupts enabled\n");
  lapic_id = lapicid();
  for (idx = 0; idx < ncpu; ++idx)
    if (cpus[idx].apicid == lapic_id) return &cpus[idx];
  panic("mycpu: unrecognized apicid\n");
}

struct proc*
myproc(void)
{
  struct cpu *c; struct proc *p;
  pushcli(); c = mycpu(); p = c->proc; popcli();
  return p;
}

static struct proc*
alloc_new_proc(void)
{
  struct proc *entry; char *sp;
  acquire(&proctab.lk);
  for (entry = proctab.proc; entry < &proctab.proc[NPROC]; entry++)
    if (entry->state == UNUSED) goto slot_found;
  release(&proctab.lk); return 0;
slot_found:
  entry->state = EMBRYO; entry->pid = pid_counter++; entry->is_thread = 0;
  release(&proctab.lk);
  if ((entry->kstack = kalloc()) == 0) { entry->state = UNUSED; return 0; }
  sp = entry->kstack + KSTACKSIZE;
  sp -= sizeof *entry->tf; entry->tf = (struct trapframe*)sp;
  sp -= 4; *(uint*)sp = (uint)trapret;
  sp -= sizeof *entry->context; entry->context = (struct context*)sp;
  memset(entry->context, 0, sizeof *entry->context);
  entry->context->eip = (uint)forkret;
  return entry;
}

void
userinit(void)
{
  struct proc *p; extern char _binary_initcode_start[], _binary_initcode_size[];
  p = alloc_new_proc(); root_proc = p;
  if ((p->pgdir = setupkvm()) == 0) panic("userinit: oom");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE; memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE<<3)|DPL_USER; p->tf->ds = (SEG_UDATA<<3)|DPL_USER;
  p->tf->es = p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF; p->tf->esp = PGSIZE; p->tf->eip = 0;
  safestrcpy(p->name,"initcode",sizeof(p->name)); p->cwd = namei("/");
  acquire(&proctab.lk); p->state = RUNNABLE; release(&proctab.lk);
}

int
growproc(int n)
{
  uint sz; struct proc *cur = myproc(); sz = cur->sz;
  if (n>0) { if((sz=allocuvm(cur->pgdir,sz,sz+n))==0) return -1; }
  else if (n<0) { if((sz=deallocuvm(cur->pgdir,sz,sz+n))==0) return -1; }
  cur->sz = sz; switchuvm(cur); return 0;
}

int
fork(void)
{
  int fd_idx, new_pid; struct proc *child; struct proc *cur = myproc();
  if ((child = alloc_new_proc()) == 0) return -1;
  if ((child->pgdir = copyuvm(cur->pgdir, cur->sz)) == 0) {
    kfree(child->kstack); child->kstack = 0; child->state = UNUSED; return -1;
  }
  child->sz = cur->sz; child->parent = cur; *child->tf = *cur->tf;
  child->is_thread = 0; child->tf->eax = 0;
  for (fd_idx=0; fd_idx<NOFILE; fd_idx++)
    if (cur->ofile[fd_idx]) child->ofile[fd_idx] = filedup(cur->ofile[fd_idx]);
  child->cwd = idup(cur->cwd);
  safestrcpy(child->name, cur->name, sizeof(cur->name));
  new_pid = child->pid;
  acquire(&proctab.lk); child->state = RUNNABLE; release(&proctab.lk);
  return new_pid;
}

/*
 * clone() - PART A TASK 1
 * Creates a kernel-managed user thread sharing the parent address space.
 * Thread entry: fcn(arg) with caller-supplied stack.
 */
int
clone(void(*fcn)(void*), void *arg, void *stack)
{
  int fd_idx, tpid; struct proc *t; struct proc *cur = myproc();
  if ((uint)stack % 4 != 0) return -1;
  if ((t = alloc_new_proc()) == 0) return -1;
  t->pgdir = cur->pgdir; t->sz = cur->sz; t->parent = cur; t->is_thread = 1;
  *t->tf = *cur->tf;
  uint usp = (uint)stack;
  usp -= 4; *(uint*)usp = (uint)arg;
  usp -= 4; *(uint*)usp = 0xffffffff;
  t->tf->esp = usp; t->tf->eip = (uint)fcn;
  for (fd_idx=0; fd_idx<NOFILE; fd_idx++)
    if (cur->ofile[fd_idx]) t->ofile[fd_idx] = filedup(cur->ofile[fd_idx]);
  t->cwd = idup(cur->cwd);
  safestrcpy(t->name, cur->name, sizeof(cur->name));
  tpid = t->pid;
  acquire(&proctab.lk); t->state = RUNNABLE; release(&proctab.lk);
  return tpid;
}

/*
 * join() - PART A TASK 1
 * Reaps a thread child; does NOT free pgdir (shared with parent).
 */
int
join(void)
{
  struct proc *child; int found_thread, reaped_pid; struct proc *cur = myproc();
  acquire(&proctab.lk);
  for(;;) {
    found_thread = 0;
    for (child=proctab.proc; child<&proctab.proc[NPROC]; child++) {
      if (child->parent != cur || !child->is_thread) continue;
      found_thread = 1;
      if (child->state == ZOMBIE) {
        reaped_pid = child->pid; kfree(child->kstack); child->kstack = 0;
        child->pid = 0; child->parent = 0; child->name[0] = 0;
        child->killed = 0; child->is_thread = 0; child->state = UNUSED;
        release(&proctab.lk); return reaped_pid;
      }
    }
    if (!found_thread || cur->killed) { release(&proctab.lk); return -1; }
    sleep(cur, &proctab.lk);
  }
}

void
exit(void)
{
  struct proc *cur = myproc(); struct proc *child; int fd;
  if (cur == root_proc) panic("init cannot exit");
  for (fd=0; fd<NOFILE; fd++) {
    if (cur->ofile[fd]) { fileclose(cur->ofile[fd]); cur->ofile[fd] = 0; }
  }
  begin_op(); iput(cur->cwd); end_op(); cur->cwd = 0;
  acquire(&proctab.lk); do_wakeup(cur->parent);
  for (child=proctab.proc; child<&proctab.proc[NPROC]; child++) {
    if (child->parent == cur) {
      child->parent = root_proc;
      if (child->state == ZOMBIE) do_wakeup(root_proc);
    }
  }
  cur->state = ZOMBIE; sched(); panic("zombie exit");
}

int
wait(void)
{
  struct proc *child; int found_child, reaped_pid; struct proc *cur = myproc();
  acquire(&proctab.lk);
  for(;;) {
    found_child = 0;
    for (child=proctab.proc; child<&proctab.proc[NPROC]; child++) {
      if (child->parent != cur) continue;
      found_child = 1;
      if (child->state == ZOMBIE) {
        reaped_pid = child->pid; kfree(child->kstack); child->kstack = 0;
        if (!child->is_thread) freevm(child->pgdir);
        child->pid = 0; child->parent = 0; child->name[0] = 0;
        child->killed = 0; child->is_thread = 0; child->state = UNUSED;
        release(&proctab.lk); return reaped_pid;
      }
    }
    if (!found_child || cur->killed) { release(&proctab.lk); return -1; }
    sleep(cur, &proctab.lk);
  }
}

void
scheduler(void)
{
  struct proc *p; struct cpu *c = mycpu(); c->proc = 0;
  for(;;) {
    sti(); acquire(&proctab.lk);
    for (p=proctab.proc; p<&proctab.proc[NPROC]; p++) {
      if (p->state != RUNNABLE) continue;
      c->proc = p; switchuvm(p); p->state = RUNNING;
      swtch(&(c->scheduler), p->context); switchkvm(); c->proc = 0;
    }
    release(&proctab.lk);
  }
}

void sched(void)
{
  int saved; struct proc *p = myproc();
  if (!holding(&proctab.lk)) panic("sched: lock");
  if (mycpu()->ncli != 1) panic("sched: ncli");
  if (p->state == RUNNING) panic("sched: running");
  if (readeflags()&FL_IF) panic("sched: interrupts");
  saved = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = saved;
}

void yield(void) { acquire(&proctab.lk); myproc()->state=RUNNABLE; sched(); release(&proctab.lk); }

void forkret(void) {
  static int first=1; release(&proctab.lk);
  if (first) { first=0; iinit(ROOTDEV); initlog(ROOTDEV); }
}

void
sleep(void *channel, struct spinlock *lk)
{
  struct proc *p = myproc();
  if (p==0) panic("sleep: no proc"); if (lk==0) panic("sleep: no lock");
  if (lk != &proctab.lk) { acquire(&proctab.lk); release(lk); }
  p->chan = channel; p->state = SLEEPING; sched(); p->chan = 0;
  if (lk != &proctab.lk) { release(&proctab.lk); acquire(lk); }
}

static void do_wakeup(void *channel) {
  struct proc *p;
  for (p=proctab.proc; p<&proctab.proc[NPROC]; p++)
    if (p->state==SLEEPING && p->chan==channel) p->state=RUNNABLE;
}

void wakeup(void *channel) { acquire(&proctab.lk); do_wakeup(channel); release(&proctab.lk); }

int kill(int pid) {
  struct proc *p; acquire(&proctab.lk);
  for (p=proctab.proc; p<&proctab.proc[NPROC]; p++) {
    if (p->pid==pid) { p->killed=1; if(p->state==SLEEPING) p->state=RUNNABLE; release(&proctab.lk); return 0; }
  }
  release(&proctab.lk); return -1;
}

void procdump(void) {
  static char *sn[] = { [UNUSED]"unused",[EMBRYO]"embryo",[SLEEPING]"sleep ",[RUNNABLE]"runble",[RUNNING]"run   ",[ZOMBIE]"zombie" };
  int i; struct proc *p; char *st; uint pcs[10];
  for (p=proctab.proc; p<&proctab.proc[NPROC]; p++) {
    if (p->state==UNUSED) continue;
    st = (p->state>=0&&p->state<NELEM(sn)&&sn[p->state]) ? sn[p->state] : "???";
    cprintf("%d %s %s",p->pid,st,p->name);
    if (p->state==SLEEPING) { getcallerpcs((uint*)p->context->ebp,pcs); for(i=0;i<10&&pcs[i];i++) cprintf(" %p",pcs[i]); }
    cprintf("\n");
  }
}