#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <libc.h>

#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>



FILE *temp_file;
void leave(int sig);

main() {
  //        (void) signal(SIGINT,leave);
          (void) signal(SIGWINCH,leave);
        for(;;) {
                /*
                 * Do things....
                 */
                printf("Ready...\n");
                (void)getchar();
        }
        /* can't get here ... */
        exit(EXIT_SUCCESS);
}

/*
 * on receipt of SIGINT, close tmp file
 * but beware - calling library functions from a
 * signal handler is not guaranteed to work in all
 * implementations.....
 * this is not a strictly conforming program
 */

void
leave(int sig) {
  //fprintf(temp_file,"\nInterrupted..\n");
  //    fclose(temp_file);

  struct winsize ws;
   int fd=open("/dev/tty",O_RDWR);

   if (ioctl(fd,TIOCGWINSZ,&ws)!=0) {
      perror("ioctl(/dev/tty,TIOCGWINSZ)");
      exit(sig);

   }
   printf(" %i,%i",ws.ws_col, ws.ws_row);
   puts(" ");
  // 
}
