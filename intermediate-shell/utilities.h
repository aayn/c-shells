#ifndef UTILITIES
#define UTILITIES
#define node struct node
#define bode struct bode
#include <stdio.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
typedef struct process
{
//  struct process *next;       /* next process in pipeline */
  char **argv;                /* for exec */
  pid_t pid;
  pid_t pgid;                  /* process ID */
  char completed;             /* true if process has completed */
  char stopped;               /* true if process has stopped */
  int status;                 /* reported status value */
} process;

bode
{
	process P;
	int list_rank;
};

node
{
	int list_rank;
	process P;
	node* next;
};

#endif