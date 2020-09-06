#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;
struct proc* prev;
struct proc* now;
uint tick;
struct
{
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;
void
tvinit(void)  
{
  int i;
  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
  SETGATE(idt[128],1,SEG_KCODE<<3,vectors[128],DPL_USER);
  initlock(&tickslock, "time");
  prev = myproc();
  tick = 0;
}
void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}
//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
   if(myproc()->killed)      
       exit();
    return;
  }
  if(tf->trapno==128){
	cprintf("user interrupt 128 called!\n");
	if(myproc()->killed)
	    exit();
	myproc()->tf=tf;
	if(myproc()->killed)
	    exit();
	return;
  }
  switch(tf->trapno){
  #ifdef DEFAULT
  case T_IRQ0 + IRQ_TIMER:
   if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
      }
      
    lapiceoi();
	break;
  #else
  #ifdef FCFS_SCHED
  case T_IRQ0 + IRQ_TIMER:
   if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      now = myproc();
      if(prev != now){
        tick=0;
      }
      else{
        tick++;
        if(tick==100 && myproc()!=0)
        {
          myproc()->killed = 1;
          cprintf("killed process %d\n",myproc()->pid);
        }
      }
      prev = now;
      wakeup(&ticks);
      release(&tickslock);
      }
    lapiceoi();
	break;
#else
#ifdef MLFQ_SCHED
  case T_IRQ0 + IRQ_TIMER:
   if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
      }
      
    lapiceoi();
    break;
#endif
#endif
#endif
case T_IRQ0 + IRQ_IDE:
  ideintr();
  lapiceoi();
  break;
case T_IRQ0 + IRQ_IDE + 1:
  // Bochs generates spurious IDE1 interrupts.
  break;
case T_IRQ0 + IRQ_KBD:
  kbdintr();
  lapiceoi();
  break;
case T_IRQ0 + IRQ_COM1:
  uartintr();
  lapiceoi();
  break;
case T_IRQ0 + 7:
case T_IRQ0 + IRQ_SPURIOUS:
  cprintf("cpu%d: spurious interrupt at %x:%x\n",
          cpuid(), tf->cs, tf->eip);
  lapiceoi();
  break;
//PAGEBREAK: 13
default:
  if (myproc() == 0 || (tf->cs & 3) == 0)
  {
    // In kernel, it must be our mistake.
    cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
            tf->trapno, cpuid(), tf->eip, rcr2());
    panic("trap");
  }
  // In user space, assume process misbehaved.
  cprintf("pid %d %s: trap %d err %d on cpu %d "
          "eip 0x%x addr 0x%x--kill proc\n",
          myproc()->pid, myproc()->name, tf->trapno,
          tf->err, cpuid(), tf->eip, rcr2());
  myproc()->killed = 1;
}
// Force process exit if it has been killed and is in user space.
// (If it is still executing in the kernel, let it keep running
// until it gets to the regular system call return.)
if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
  exit();
//extern int yield_mode;
//Force process to give up CPU on clock tick.
//If interrupts were on while locks held, would need to check nlock.
// if(myproc() && myproc()->state == RUNNING &&
//    tf->trapno == T_IRQ0+IRQ_TIMER && !yield_mode)  yield();
#ifdef DEFAULT
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();
#else
#ifdef FCFS_SCHED
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
    yield();
#else
#ifdef MLFQ_SCHED
  if (myproc() && myproc()->state == RUNNING &&
      tf->trapno == T_IRQ0 + IRQ_TIMER)
  {
    myproc()->ticks++;
    if (myproc()->lev == 0 && myproc()->ticks == 4)
    {
      //cprintf("down\n");
      myproc()->lev++;
      myproc()->ticks = 0;
    }
    else if (myproc()->lev == 1 && myproc()->ticks == 8)
    {
      myproc()->ticks = 0;
      if(myproc()->priority>0)
        myproc()->priority--;
    }
    yield();
  }
  if (ticks != 0 && ticks % 100 == 0)
  {
    //cprintf("boost\n");
    struct proc *p;
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p)
      {
        p->lev = 0;
        p->ticks = 0;
      }
    }
    release(&ptable.lock);
  }
#endif
#endif
#endif
  // Check if the process has been killed since we yielded
  if (myproc() && myproc()->killed && (tf->cs & 3) == DPL_USER)
    exit();
}