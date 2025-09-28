#include <user/user.h>

int main() {
  int f2c[2];  // father to child process pipe, father write, child read
  int c2f[2];  // child to father process pipe, child write, father read
  char father_pid_str[64];
  char child_pid_str[64];
  if (pipe(f2c) < 0) {
    fprintf(2, "unable to create pipe\n");
    exit(1);
  }
  if (pipe(c2f) < 0) {
    fprintf(2, "unable to create pipe\n");
  }
  if (fork() == 0) {
    // Child process
    close(f2c[1]);
    close(c2f[0]);
    int c_pid = getpid();
    itoa(c_pid, child_pid_str);
    int c_len = strlen(child_pid_str);
    read(f2c[0], father_pid_str, 63);
    fprintf(1, child_pid_str);                // stdout
    fprintf(1, ": received ping from pid ");  // stdout
    fprintf(1, father_pid_str);               // stdout
    printf("\n");

    // starts to write to father
    write(c2f[1], child_pid_str, c_len);
    exit(0);
  }
  close(f2c[0]);
  close(c2f[1]);
  int f_pid = getpid();
  itoa(f_pid, father_pid_str);
  int f_len = strlen(father_pid_str);
  write(f2c[1], father_pid_str, f_len);  // I think we should pass father_pid(f_len byte(s) of data) to child
  // start to wait for child(blocked because c2f pipe is empty now)
  read(c2f[0], child_pid_str, 63);          // since we do't know exactly how long the child_pid_str is.
  fprintf(1, father_pid_str);               // stdout
  fprintf(1, ": received pong from pid ");  // stdout
  fprintf(1, child_pid_str);                // stdout
  printf("\n");
  exit(0);
}