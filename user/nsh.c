#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

// redirect stdin(0)/stdout(1) to pipe
void 
redirect(int dir, int pd[])
{
  close(dir);
  dup(pd[dir]);
  close(pd[0]);
  close(pd[1]);
}

// split by pipe character |, return the right half
// or return NULL if not pipe
char*
splitpipe(char *cmd)
{
  for(; *cmd && *cmd != '|'; cmd++)
    ;
  if(*cmd == '|'){
    *cmd++ = '\0';
    return cmd;
  }
  return 0;
}

int
isspace(char c)
{
  return c == ' ' || c == '\t' || c == '\n';
}

void
execcmd(char *cmd)
{
  // split in to words
  char *argv[MAXARG];
  int argc = 0, inpos = 0, outpos = 0;
  for(; *cmd; *cmd++ = '\0'){
    // if the begining of word
    if(!isspace(*cmd)){
      argv[argc++] = cmd;
      if(*cmd == '<') inpos = argc;
      if(*cmd == '>') outpos = argc;
      // skip the whole word
      for(; *cmd && !isspace(*cmd); cmd++)
        ;
      if(!*cmd){
        break;
      }
    }
  }
  argv[argc] = 0;
  if(inpos){
    argv[argc=inpos-1] = 0;
    close(0);
    open(argv[inpos], O_RDONLY);
  }
  if(outpos){
    argv[argc=outpos-1] = 0;
    close(1);
    open(argv[outpos], O_CREATE | O_WRONLY);
  }
  if(argc == 0){
    exit(0);
  }
  exec(argv[0], argv);
  fprintf(2, "argv:");
  for(int i = 0; argv[i]; i++){
    fprintf(2, " %s", argv[i]);
  }
  fprintf(2, "\n");
}

void
parsecmd(char *cmd)
{
  char *nextcmd = splitpipe(cmd);
  if(!nextcmd){
    execcmd(cmd);
    fprintf(2, "exec: %s failed\n", cmd);
    exit(0);
  }
  int pd[2];
  pipe(pd);
  if(fork()){
    redirect(1, pd);
    execcmd(cmd);
    fprintf(2, "exec: %s failed\n", cmd);
  } else if(fork()) {
    redirect(0, pd);
    parsecmd(nextcmd);
  }
  close(pd[0]);
  close(pd[1]);
  wait(0);
  wait(0);
  exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  fprintf(2, "@ ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  while(getcmd(buf, sizeof buf) == 0){
    if(fork()){
      wait(0);
    } else {
      parsecmd(buf);
      fprintf(2, "parsecmd not exit\n");
    }
  }
  exit(0);
}