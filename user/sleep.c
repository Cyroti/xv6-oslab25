// please change it!
#include <user/user.h>

int main(int argc, char *argv[]) {
  int i;
  if (argc <= 1) {
    fprintf(2, "sleep: missing operand\nusage: sleep 1 2 3,...\n");
    exit(1);
  }

  int t = 0;
  for (i = 1; i < argc; i++) {
    t += atoi(argv[i]);
  }
  sleep(t);

  exit(0);
}