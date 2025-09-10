#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mutex.h"

int
sys_fork(void)
{
  return fork();
}

int sys_clone(void)
{
  int fn, stack, arg;
  argint(0, &fn);
  argint(1, &stack);
  argint(2, &arg);
  return clone((void (*)(void*))fn, (void*)stack, (void*)arg);
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
  addr = myproc()->sz;
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
  if (n == 0) {
    yield();
    return 0;
  }
  acquire(&tickslock);
  ticks0 = ticks;
  myproc()->sleepticks = n;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  myproc()->sleepticks = -1;
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

// Newly added system calls for P5
int
sys_macquire(void) {
  mutex *m;

  // Get argument
  if (argptr(0, (void*)&m, sizeof(*m)) < 0) {
	return -1;
  }

  struct proc *p = myproc();
  pte_t* v_addr = walkpgdir(p->pgdir, (const void*)m, 0);

  // Try to acquire the spinlock within the mutex
  acquire(&m->lk);

  if(p->num_locks == 16)
    return -1; // No space for another lock, process has 16/16 locks
  p->locks_held[p->num_locks] = V2P(v_addr);
  p->num_locks++;

  while(m->locked == 1) {
	  sleep(m, &m->lk);
  }
  m->locked = 1;
  release(&m->lk);	// Protects the sleeplock itself
  
  return 0;  
}

int
sys_mrelease(void) {
  mutex *m;
 
  // Get argument
  if (argptr(0, (void*)&m, sizeof(*m)) < 0) {
	return -1;
  }

  struct proc *p = myproc();
  pte_t* v_addr = walkpgdir(p->pgdir, (const void*)m, 0);
  uint p_addr = V2P(v_addr);

  // Unlock
  acquire(&m->lk);
  m->locked = 0;
  

  int i;
  for(i = 0; i < MAXNUMLOCKS; i++){ // Find position of lock to remove
    if(p->locks_held[i] == p_addr){
      for(; i < p->num_locks - 1; i++){ // Remove lock and shift others down
        p->locks_held[i] = p->locks_held[i + 1];
      }
      p->locks_held[i] = 0;
      break;
    }
  }
  p->num_locks--;

  wakeup(m);
  release(&m->lk);
  return 0;
}

int
sys_nice(void) {
  // Get the running process
  int inc;
  struct proc *p = myproc();

  // Check for argument
  if (argint(0, &inc) < 0) {
	  return -1;
  }

  // Add the inc value into the running thread's priority
  int new_nice = p->nice + inc;

  // Check if positive or negative saturation is necessary
  p->nice = (p->nice > 19) ? 19 : new_nice;
  p->nice = (p->nice < -20) ? -20 : new_nice;
  
  // Success	
  return 0;
}
