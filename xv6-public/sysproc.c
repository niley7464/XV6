#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->master->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getppid(void)
{
	return myproc()->parent->pid;
}

//int yield_mode;

int
sys_yield(void)
{
   // yield_mode = 1;
   
    #ifdef MLFQ_SCHED
    myproc()->ticks=0;
    #endif
    yield();
    return 0;
}
int
sys_getlev(void)
{
  if(myproc()->monopolize) return 0;
  return myproc()->lev;
}

int
sys_setpriority(int pid, int priority)
{
  argint(0,&pid);
  argint(1,&priority);
  struct proc* p;
  acquire(&ptable.lock);
   for (p = ptable.proc; p < &ptable.proc[NPROC] && p->pid != 0; p++)
    {
      if (p->pid == pid){
        p->priority = priority;
        break;
        }
    }
  release(&ptable.lock);
  return 0;
}

int sys_monopolize(int password)
{
  argint(0, &password);
  if (password != 2017030237)
  {
    cprintf("password does not match.");
    kill(myproc()->pid);
    pushcli();
    if (mycpu()->isMonopolized)
      mycpu()->isMonopolized = 0;
    popcli();
    return 0;
  }
  pushcli();
  if (mycpu()->isMonopolized){
    mycpu()->isMonopolized = 0;
    myproc()->monopolize = 0;
    myproc()->lev=0;
    myproc()->priority = 0;
  }
  else{
     mycpu()->isMonopolized = 1;
    myproc()->monopolize = 1;
    myproc()->lev=0;
    myproc()->priority = 0;
  }
  popcli();
  return 0;
}

int sys_thread_create(void){
   int thread, start_routine, arg;

  if(argint(0, &thread) < 0)
      return -1;
  
  if(argint(1, &start_routine) < 0)
      return -1;
  
  if(argint(2, &arg) < 0)
      return -1;
      
  return thread_create((thread_t *)thread, (void *)start_routine,(void*)arg);
}

int sys_thread_exit(void){
  int retval;
  
  if(argint(0, &retval) < 0)
      return -1;

  thread_exit((void*)retval);
  return 0;
}

int sys_thread_join(void){
  int thread, retval;
  
  if(argint(0, &thread) < 0)
      return -1;

  if(argint(1, &retval) < 0)
      return -1;
  
  return thread_join((thread_t)thread,(void**)retval);
}