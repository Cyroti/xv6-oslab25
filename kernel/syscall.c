#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int fetchaddr(uint64 addr, uint64 *ip) {
  struct proc *p = myproc();
  if (addr >= p->sz || addr + sizeof(uint64) > p->sz) return -1;
  if (copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0) return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int fetchstr(uint64 addr, char *buf, int max) {
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if (err < 0) return err;
  return strlen(buf);
}

static uint64 argraw(int n) {
  struct proc *p = myproc();
  switch (n) {
    case 0:
      return p->trapframe->a0;
    case 1:
      return p->trapframe->a1;
    case 2:
      return p->trapframe->a2;
    case 3:
      return p->trapframe->a3;
    case 4:
      return p->trapframe->a4;
    case 5:
      return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip) {
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int argaddr(int n, uint64 *ip) {
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int argstr(int n, char *buf, int max) {
  uint64 addr;
  if (argaddr(n, &addr) < 0) return -1;
  return fetchstr(addr, buf, max);
}

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
extern uint64 sys_rename(void);
extern uint64 sys_trace(void);

static uint64 (*syscalls[])(void) = {
    [SYS_fork] = sys_fork,   [SYS_exit] = sys_exit,     [SYS_wait] = sys_wait,     [SYS_pipe] = sys_pipe,
    [SYS_read] = sys_read,   [SYS_kill] = sys_kill,     [SYS_exec] = sys_exec,     [SYS_fstat] = sys_fstat,
    [SYS_chdir] = sys_chdir, [SYS_dup] = sys_dup,       [SYS_getpid] = sys_getpid, [SYS_sbrk] = sys_sbrk,
    [SYS_sleep] = sys_sleep, [SYS_uptime] = sys_uptime, [SYS_open] = sys_open,     [SYS_write] = sys_write,
    [SYS_mknod] = sys_mknod, [SYS_unlink] = sys_unlink, [SYS_link] = sys_link,     [SYS_mkdir] = sys_mkdir,
    [SYS_close] = sys_close, [SYS_trace] = sys_trace,
};

static const char *name_syscalls[] = {
    [SYS_fork] = "sys_fork",   [SYS_exit] = "sys_exit",     [SYS_wait] = "sys_wait",     [SYS_pipe] = "sys_pipe",
    [SYS_read] = "sys_read",   [SYS_kill] = "sys_kill",     [SYS_exec] = "sys_exec",     [SYS_fstat] = "sys_fstat",
    [SYS_chdir] = "sys_chdir", [SYS_dup] = "sys_dup",       [SYS_getpid] = "sys_getpid", [SYS_sbrk] = "sys_sbrk",
    [SYS_sleep] = "sys_sleep", [SYS_uptime] = "sys_uptime", [SYS_open] = "sys_open",     [SYS_write] = "sys_write",
    [SYS_mknod] = "sys_mknod", [SYS_unlink] = "sys_unlink", [SYS_link] = "sys_link",     [SYS_mkdir] = "sys_mkdir",
    [SYS_close] = "sys_close", [SYS_trace] = "sys_trace",
};

// 00=int  01=str  10=ptr  11=nil
#define ARG_INT 0x00
#define ARG_STR 0x01
#define ARG_PTR 0x10
#define ARG_NIL 0x11

typedef struct {
  uint8 cnt;    // 参数个数
  uint8 ty[6];  // 各参数类型
} sig_t;

static const sig_t sig_syscalls[] = {[SYS_fork] = {0},
                                     [SYS_exit] = {1, {ARG_INT}},
                                     [SYS_wait] = {1, {ARG_PTR}},
                                     [SYS_kill] = {2, {ARG_INT, ARG_INT}},
                                     [SYS_exec] = {2, {ARG_STR, ARG_PTR}},
                                     [SYS_getpid] = {0},
                                     [SYS_sleep] = {1, {ARG_INT}},
                                     [SYS_sbrk] = {1, {ARG_INT}},
                                     [SYS_open] = {2, {ARG_STR, ARG_INT}},
                                     [SYS_write] = {3, {ARG_INT, ARG_PTR, ARG_INT}},
                                     [SYS_read] = {3, {ARG_INT, ARG_PTR, ARG_INT}},
                                     [SYS_close] = {1, {ARG_INT}},
                                     [SYS_dup] = {1, {ARG_INT}},
                                     [SYS_pipe] = {1, {ARG_PTR}},
                                     [SYS_chdir] = {1, {ARG_STR}},
                                     [SYS_mkdir] = {1, {ARG_STR}},
                                     [SYS_mknod] = {3, {ARG_STR, ARG_INT, ARG_INT}},
                                     [SYS_fstat] = {2, {ARG_INT, ARG_PTR}},
                                     [SYS_link] = {2, {ARG_STR, ARG_STR}},
                                     [SYS_unlink] = {1, {ARG_STR}},
                                     [SYS_uptime] = {0},
                                     [SYS_trace] = {1, {ARG_INT}}};

void syscall(void) {
  int num, trace_mask, pid;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  pid = p->pid;
  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    /* 1. 快照：把 a0-a5 先捞出来，避免执行后 trapframe 被覆盖 */
    uint64 raw[6];
    for (int i = 0; i < 6; i++) raw[i] = argraw(i);
    /* 2. 执行系统调用 */
    p->trapframe->a0 = syscalls[num]();
    trace_mask = p->trace_mask;  // update the mask, so trace can trace itself
    int ret = p->trapframe->a0;
    if (((trace_mask >> num) & 0x1) == 0x1) {  // trace can trace itself now , since the mask has changed
      /* 3. 用快照打印 */
      const sig_t *s = &sig_syscalls[num];
      printf("%d: %s(", pid, name_syscalls[num]);
      if (s->cnt == 0) printf("void");  // void stands for no parameters

      for (int i = 0; i < s->cnt; i++) {
        uint8 ty = s->ty[i];
        switch (ty) {
          case ARG_INT:
            printf("%d", (int)raw[i]);
            break;
          case ARG_PTR:
            printf("%p", (void *)raw[i]);
            break;
          case ARG_STR: {
            char tmp[64];
            if (fetchstr(raw[i], tmp, sizeof(tmp)) < 0)
              printf("<bad_str>");
            else
              // printf("%s", tmp); //we should print int
              printf("%d", raw[i]);
            break;
          }
          default:
            printf("<?>");
        }
        if (i >= 0) break;  // according to the requirements, we only print the arg0
        if (i != s->cnt - 1) printf(", ");
      }
      printf(") -> %d\n", ret);
    }
  } else {
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
