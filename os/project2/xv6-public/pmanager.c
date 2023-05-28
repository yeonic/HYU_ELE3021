#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXLINE 1000
 
int nextcmd(char* buf, int size);
int buftoi(char* buf, int* sidx);

int
main() 
{
   int pid, idx;
   char buf[MAXLINE];

   while (nextcmd(buf, MAXLINE) > 0) {
      if(buf[0] == '\n') {
         // case0: early continue
         continue;
      } else if(buf[0] == 'l' && buf[1] == 'i' && buf[2] == 's' && buf[3] == 't') {
         // case1: list
         printplist();
      } else if(buf[0] == 'k' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'l' && buf[4] == ' ') {
         // case2: kill
         idx = 5;
         if(buf[0] == ' ' || buf[1] == '\n') {
            printf(2, "pmanager: you need to put pid.\n");
            continue;
         }

         // atoi(pid)
         pid = buftoi(buf, &idx);

         if(kill(pid) == -1)
            printf(2, "pmanager: failed to kill process(%d)\n", pid);
         else
            printf(2, "pmanager: killed process(%d)\n", pid);
            
      } else if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'e' && buf[3] == 'c' && buf[4] == 'u' && buf[5] == 't' && buf[6] == 'e' && buf[7] == ' ') {
         // case3: execute
         idx = 8;
         int stacksize, pathidx, pid;
         char path[MAXLINE];

         stacksize = pathidx = 0; 

         while(buf[idx] != ' ' && buf[idx] != '\n'){
            path[pathidx++] = buf[idx++];
         }
         path[pathidx] = '\0';
         printf(2, "path: %s\n", path);

         if(buf[idx++] == '\n') {
            printf(2, "pmanager: stacksize required.\n");
            continue;
         }

         stacksize = buftoi(buf, &idx);
         printf(2, "stacksize: %d\n", stacksize);

         pid = fork();
         if(pid == 0) {
            char* argv[] = {path, 0};
            if(stacksize == 1)
               exec(argv[0], argv);
            else
               exec2(argv[0], argv, stacksize);
            printf(2, "pmanager: failed to exec.\n", stacksize);
         } else if(pid > 0) {
            wait();
         } else {
            printf(2, "pmanager: failed to fork.\n");
            continue;
         }

      } else if(buf[0] == 'm' && buf[1] == 'e' && buf[2] == 'm' && buf[3] == 'l' && buf[4] == 'i' && buf[5] == 'm' && buf[6] == ' ' ) {
         // case4: memlim
         idx = 7;
         int memlim = 0;
         pid = buftoi(buf, &idx);

         if(buf[idx++] != ' ') {
            printf(2, "pmanager: memlimit is integer.\n");
            continue;
         }
         
         if(buf[idx] == '-') {
            printf(2, "pmanager: memlimit needs to be positive.\n");
            continue;
         }

         memlim = buftoi(buf, &idx);
         if(setmemorylimit(pid, memlim) < 0)
            printf(2, "pmanager: failed to set memlim of process(%d)\n", pid);
         else
            printf(2, "pmanager: setting memlim of process(%d) done.\n", pid);

      } else if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't') {
         // case5: exit
         break;
      } else {
         // case default: wrong command.
         printf(2, "pmanager: %s is not a pmanager command. Try again.\n", buf);
      }
   }
   exit();
}

int 
nextcmd(char* buf, int size)
 {
    printf(2, "pmanager> ");
    memset(buf, 0, size);
    gets(buf, size);
    if(buf[0] == 0)
      return -1;
   return 1;
 }

 int
 buftoi(char* buf, int* sidx)
 {
   int ret = 0;
   while('0' <= buf[*sidx] && buf[*sidx] <= '9') {
      ret = ret*10 + buf[*sidx] - '0';
      *sidx += 1;
    }
   return ret;
 }