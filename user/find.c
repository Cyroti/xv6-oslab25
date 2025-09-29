#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/types.h"

#define BUFF_LEN 128  // len for buff for res

void dfs(const char *dir, const char *filename) {
  int fd;
  struct stat st;  // used to denote the type of inode.
  struct dirent de;
  char buff[BUFF_LEN];
  // below I consult the implementation of ls.c
  // printf("dfs open dir:%s, long: %d\n", dir, strlen(dir));
  if ((fd = open(dir, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", dir);
    return;  // end recurrsion
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", dir);
    close(fd);
    return;
  }

  // not matter whether or not a dataFile or a directory
  const char *p = dir + strlen(dir);  // first char after last /, but now is on the end of dir, namely the '/0'
  while (p > dir && (*(p - 1)) != '/') p--;
  const char *name = p ? p : dir;
  // printf("find a filename without /: %s, long: %d\n", name, strlen(name));
  if (strcmp(name, filename) == 0) {
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!find one ans: %s\n", dir);  // find one ans
    printf("%s\n", dir);
  }

  switch (st.type) {
    case T_FILE: { /* 叶子文件：dir 就是完整路径 */
      break;
    }
    case T_DIR:
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {                         // tranverse this dir
        if (de.inum == 0) continue;                                             // deleted
        if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;  // not recur into '.' or '..'

        int dlen = strlen(dir), ndlen = strlen(de.name);
        if (dlen + 1 + DIRSIZ + 1 > BUFF_LEN) {  // including '/'
          fprintf(2, "find: too long dirname\n");
          continue;
        }
        memcpy(buff, dir, dlen);                     // dest, source, len
        if (dir[dlen - 1] != '/') buff[dlen] = '/';  // avoid .//XX
        memcpy(buff + dlen + 1, de.name, ndlen);
        buff[dlen + 1 + ndlen] = '\0';
        dfs(buff, filename);  // 递归进子目录/文件
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc <= 2) {
    fprintf(2, "find: missing operand\n");  // stderr
    exit(1);
  } else if (argc != 3) {
    fprintf(2, "usage: find . a.cc\n");
    exit(1);
  }
  char *dir = argv[1];
  char *filename = argv[2];
  char legaldir[128];
  if (dir[0] != '.' && dir[0] != '/') {  // not /XXX or . or ./XXX
    legaldir[0] = '.';
    legaldir[1] = '/';
    strcpy(legaldir + 2, dir);
  } else
    memcpy(legaldir, dir, strlen(dir));
  // printf("legaldir: %s\n", legaldir);
  dfs(legaldir, filename);
  exit(0);
}