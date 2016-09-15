#include <stdio.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <pwd.h>
#include "utilities.h"
#define TOKEN_DELIM " \t\n\r\a"

int a,*p=&a;
char execpath[1024];

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shellactive;
int exitedProcess=0;

struct utsname uD;
char HOME_DIR[1024];

/*Reads a line from stdin.*/
char* readALine()
{
	char *line;
	ssize_t buffsize=0;
	getline(&line, &buffsize, stdin);
	return line;
}

void errorHandle(char *a)
{
	fprintf(stderr, "%s\n", a);
}

/*Tokenizes the line that is read from stdin.*/
char** getTokens(char *line, char *delimiter)
{
	int buffSize=50,i=0;
	char **tokens=malloc(sizeof(char*)*buffSize);
	char *token;
	if(tokens==NULL)
		errorHandle("No memory for tokens.\n");
	token=strtok(line,delimiter);
	while(token!=NULL)
	{
		tokens[i++]=token;
		token=strtok(NULL,delimiter);
	}
	tokens[i]='\0';
	return tokens;
}

/*Checks if blank line.*/
int shellExecution(char **tokens)
{
	if(tokens[0]!=NULL)
		return processLaunch(tokens);
	return 1;
}

void resetArgv(char **argv)
{
	int i=0;
	while(argv[i])
		argv[i++]=NULL;
}

/*Check for '&' character among the tokens.*/
int checkbg(char **tokens)
{
	int i,j;
	for(i=1;tokens[i];i++)
		if(strcmp(tokens[i],"&")==0)
			return 0;
	return 1;
}

void backHandler()
{
	process P;
	//int wstat;
	//union wait wstat;
	while (1) {
		P.pid = wait3 (&P.status, WNOHANG, (struct rusage *)NULL );
		if (P.pid == 0)
			return;
		else if (P.pid == -1)
			return;
		else
			printf("[+] %d exited\n",P.pid);
	}
}

/* Launches various processes.*/
int processLaunch(char **tokens, char *argv)
{
	int waitFlag=1,i;
	process P;
	P.argv=malloc(sizeof(char*)*64);
	resetArgv(P.argv);
	waitFlag=checkbg(tokens);
	for(i=0;tokens[i];i++)
	{
		if(waitFlag==0 && strcmp(tokens[i],"&")==0)
			break;
		P.argv[i]=tokens[i];
	}
	if(strcmp(tokens[0],"echo")==0)
		return builtInEcho(P.argv);
	else if(strcmp(tokens[0],"cd")==0)
		return builtInCd(P.argv);
	else if(strcmp(tokens[0],"pwd")==0)
		return builtInPwd(P.argv,argv[0]);
	else if(strcmp(tokens[0],"pinfo")==0)
		return builtInPinfo();
	else if(strcmp(tokens[0],"exit")==0)
	{
		printf("Buh-bye.\n");
		exit(1);
	}
	signal(SIGCHLD, backHandler);
	pid_t pgid = getpgrp();
  	P.pid = fork();
  	if (P.pid == 0)
	{
		setpgid(P.pid,pgid);
   	 	if (execvp(P.argv[0],P.argv)==-1)
     		perror("aaysh");
  	}
  	else if(P.pid < 0)
    	perror("aaysh");
  	else
  	{
  		/*NOTE: This is not the best way to accomplish background processes
  		these processes can become completely independent of the shell and,
  		can lead to zombie processes.*/
  		if(waitFlag==1)
      		waitpid(P.pid, &(P.status), WUNTRACED);
      	else
      	{
      		setpgid(P.pid,P.pid);
      		exitedProcess=P.pid;
		}	
      	//printf("\nPress Enter to continue...\n(Please read the note in the comments of the source code.)\n");
  	}

  return 1;

}

int builtInEcho(char **tokens)
{
	int i,j,nlflag=1,beflag=0,wflag=0;

	for(i=1;tokens[i]!=NULL;i++)
	{
		int len=strlen(tokens[i]);
		for(j=0;j<len;j++)
		{
			if(tokens[i][j]=='-')
			{
				wflag=1;
				if(strcmp(tokens[i],"-ne")==0)
					{nlflag=0;beflag=1;j+=3;}
				else if(strcmp(tokens[i],"-nE")==0)
					{nlflag=0;beflag=0;j+=3;}
				else if(strcmp(tokens[i],"-en")==0)
					{nlflag=0;beflag=1;j+=3;}
				else if(strcmp(tokens[i],"-En")==0)
					{nlflag=0;beflag=0;j+=3;}
				else if(strcmp(tokens[i],"-eE")==0)
					{beflag=0;j+=2;}
				else if(strcmp(tokens[i],"-Ee")==0)
					{beflag=1;j+=2;}
				else if(strcmp(tokens[i],"-n")==0)
					{nlflag=0;j+=2;}
				else if(strcmp(tokens[i],"-e")==0)
					{beflag=1;j+=2;}
				else if(strcmp(tokens[i],"-E")==0)
					{beflag=0;j+=2;}
				else
					wflag=0;
			}
			if(tokens[i][j]=='"')
				printf("");
			else if(tokens[i][j]=='\\' && tokens[i][j+1] && tokens[i][j+1]=='\\')
				{printf("\\");j++;}
			else if(tokens[i][j]=='\\' && (!tokens[i][j+1] || tokens[i][j+1]!='\\'))
				printf("");
			else
				printf("%c",tokens[i][j]);
		}
		if(wflag==0)
			printf(" ");
	}
	if(nlflag==1)
		printf("\n");
	return 1;
}

int builtInCd(char **tokens)
{
	if(tokens[1]==NULL)
		errorHandle("\"cd\" where not specified.");
	else if(chdir(tokens[1])!=0)
		perror("aaysh");
	return 1;
}

int builtInPwd()
{
	char A[1024];
	getcwd(A,sizeof(A));
	if(!A)
		perror("aaysh");
	printf("%s\n",A);
	return 1;
}

int builtInPinfo(char A[])
{
	printf("Shell pid: %d\n",shell_pgid);
	printf("Starting address(virtual): %x\n",p);
	printf("Absoulte Executable path: %s%s\n",HOME_DIR,execpath);
	return 1;
}

int customStrCpy(char b[],char a[],int n)
{
	int l=strlen(a);
	if(l<n)
		return 0;
	int i;
	for(i=0;i<n;i++)
		b[i]=a[i];
	b[i]='\0';
	return 1;
}

int customStrCat(char a[], char b[], int startIndex, int aEnd, int bEnd)
{
	int i,j=aEnd;
	for(i=startIndex;b[i] && i<bEnd;i++)
		a[j++]=b[i];
	a[j]='\0';
}

/*Main loop of the shell*/
void shellLoop(char *argv[])
{
	char *line=(char*)malloc(sizeof(char)*1024);
	int flag,hlen=strlen(HOME_DIR);
	do
	{
		flag=0;
		uname(&uD);
		int buffSize=64;
		struct passwd *p = getpwuid(getuid());
		if(p==NULL)
		{
			errorHandle( "Error: Could not get current user-id.");
			exit(1);
		}
		char *sname = strtok(uD.nodename, ".");

		char cwd[1024],**tokens=malloc(sizeof(char*)*buffSize), **tokenoftokens=malloc(sizeof(char*)*buffSize);
		getcwd(cwd, sizeof(cwd));
		char hlenCwd[1024];
		if(customStrCpy(hlenCwd,cwd,hlen) && strcmp(hlenCwd,HOME_DIR)==0)
		{
			char tcwd[1024];
			tcwd[0]='~';tcwd[1]='\0';
			customStrCat(tcwd,cwd,hlen,strlen(tcwd),strlen(cwd));
			strcpy(cwd, tcwd);
		}
		printf("<%s@%s: %s >",p->pw_name,sname,cwd);

		line=readALine();
		tokenoftokens=getTokens(line,";");
		int i;
		for(i=0;tokenoftokens[i];i++)
		{
			tokens=getTokens(tokenoftokens[i],TOKEN_DELIM);
			int j=0;
			flag=shellExecution(tokens);
			while(tokens[j])
				tokens[j++]=NULL;
			free(tokens);
		}
		free(line);
		free(tokenoftokens);
	}while(flag);
}
		
void shellInit()
{
	getcwd(HOME_DIR, sizeof(HOME_DIR));
	shell_terminal = STDIN_FILENO;
	shellactive = isatty(shell_terminal);
	if(shellactive)
	{
		while(tcgetpgrp(shell_terminal)!=(shell_pgid=getpgrp()))
			kill(-shell_pgid,SIGTTIN);
	}
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
  	signal (SIGTSTP, SIG_IGN);
  	signal (SIGTTIN, SIG_IGN);
  	signal (SIGTTOU, SIG_IGN);
  	signal (SIGCHLD, SIG_IGN);

  	shell_pgid=getpid();
  	if(setpgid(shell_pgid, shell_pgid)<0)
  	{
  		perror("Couldn't");
  		exit(1);
  	}
  	tcsetpgrp(shell_terminal,shell_pgid);
}

int main(int args, char *argv[])
{
	char *word=argv[0];
	if(argv[0][0]=='.')
		memmove(&word[0], &word[0 + 1], strlen(word) - 0);
	strcpy(execpath,argv[0]);

	shellInit();
	shellLoop(argv);

	return 0;
}