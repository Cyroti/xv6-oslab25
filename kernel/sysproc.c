#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_exit(void) {
  int n;
  if (argint(0, &n) < 0) return -1;
  exit(n);
  return 0;  // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
  uint64 p;
  if (argaddr(0, &p) < 0) return -1;
  return wait(p);
}

uint64 sys_sbrk(void) {
  int addr;
  int n;

  if (argint(0, &n) < 0) return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0) return -1;
  return addr;
}

uint64 sys_sleep(void) {
  int n;
  uint ticks0;

  if (argint(0, &n) < 0) return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64 sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0) return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64 sys_rename(void) {
  char name[16];
  int len = argstr(0, name, MAXPATH);
  if (len < 0) {
    return -1;
  }
  struct proc *p = myproc();
  memmove(p->name, name, len);
  p->name[len] = '\0';
  return 0;
}

uint64 sys_pstate(void) {
  int pid;
  uint64 running_time_ptr, runnable_time_ptr, sleeping_time_ptr;
  if (argint(0, &pid) < 0) return -1;
  if (argaddr(1, &running_time_ptr) < 0) return -1;
  if (argaddr(2, &runnable_time_ptr) < 0) return -1;
  if (argaddr(3, &sleeping_time_ptr) < 0) return -1;

  struct proc *process = getproc_plain(pid);
  if (process == 0) return -1;

  acquire(&process->lock);
  // 用临时变量减少持锁时间与重复 copyout 的竞态
  uint running  = process->running_ticks;
  uint runnable = process->runnable_ticks;
  uint sleeping = process->sleeping_ticks;
  release(&process->lock);

  if (copyout(myproc()->pagetable, running_time_ptr,  (char *)&running,  sizeof(running))  < 0) return -1;
  if (copyout(myproc()->pagetable, runnable_time_ptr, (char *)&runnable, sizeof(runnable)) < 0) return -1;
  if (copyout(myproc()->pagetable, sleeping_time_ptr, (char *)&sleeping, sizeof(sleeping)) < 0) return -1;
  return 0;
}

uint64 sys_cpustate(void) {
  uint64 uptr;
  if (argaddr(0, &uptr) < 0) return -1;

  uint tmp[NCPU];
  for (int i = 0; i < NCPU; i++) tmp[i] = cpus[i].cpu_ticks;

  // 一次性拷到用户空间
  if (copyout(myproc()->pagetable, uptr, (char *)tmp, sizeof(tmp)) < 0) return -1;
  return 0;
}

void sys_setnice(void) {}