#include "utilities.h"
#define TOKEN_DELIM " \t\n\r\a"

int a,*p=&a;
char execpath[1024];

pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shellactive;
int exitedProcess=0;

node* process_list;

pid_t to_be_listed;

struct utsname uD;
char HOME_DIR[1024];

char pro_name_array[32770][100];

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

int p_rank=0;
void add_process(process P)
{
	node *proc=(node*)malloc(sizeof(node));
	proc->P=P;
	p_rank++;
	node *t=process_list;
	while(t->next!=NULL)
		t=t->next;
	proc->list_rank=p_rank;
	t->next=proc;
	t->next->next=NULL;
}

void delete_process(pid_t pid, int sig)
{
	node *t=process_list;
	node *u=t->next;
	int found=0;
	while(u!=NULL){
		if(u->P.pid==pid){
			t->next=u->next;
			u->next=NULL;
			free(u);
			kill(u->P.pid,sig);
			found=1;
			break;
		}
		t=u;
		u=u->next;
	}
	if(found==0)
		printf("process with pid %d not present.\n",pid);
}

node* display_process();

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

void resetArgv(char **argv)
{
	int i=0;
	while(argv[i])
		argv[i++]=NULL;
}

/*Check for '&' character among the tokens.*/
int check_special_char(char **tokens, char a[])
{
	int i,j;
	//printf("%s\n\n",r);
	for(i=1;tokens[i];i++)
		if(strcmp(tokens[i],a)==0)
			return 0;
	return 1;
}

void backHandler()
{
	process P;
	while (1) {
		P.pid = wait3 (&P.status, WNOHANG, (struct rusage *)NULL );
		if (P.pid == 0)
			return;
		else if (P.pid == -1)
			return;
		else
			delete_process(P.pid,9);
			printf("[+] %d exited\n",P.pid);
	}
}

int redir_validator(char **tokens)
{
	if(token_count(tokens)<3)
	{
		printf("sgfhds\n");
		return 0;
	}
	return 1;	
}

int token_number(char **tokens, char r[])
{
	int i=0,p=0;
	for(i=0;tokens[i];i++)
	{
		if(strcmp(tokens[i],r)==0)
			p=i;
	}
	if(p==i)
		return -1;
	return p;
}

char* pname;
void ctrlZ()
{
	process P;
	P.pid=to_be_listed;
	setpgid(P.pid,P.pid);
	add_process(P);
	kill(P.pid, SIGTSTP);
}


/* Launches various processes.*/
int processLaunch(int pipe_flag,char **over_tokens)
{
	char **tokens;
    tokens=malloc(sizeof(char*)*640);	
	if(!pipe_flag)
		tokens=over_tokens;
	int waitFlag=1,i;
	int riFlag=0,roFlag=0;
	waitFlag=check_special_char(tokens,"&");
	if(!pipe_flag){
		pname=strdup(tokens[0]);
		if(strcmp(tokens[0],"cd")==0){
			builtInCd(tokens);
			return 1;
		}
	}
  	signal(SIGTSTP, ctrlZ);
	signal(SIGCHLD, backHandler);
	pid_t pgid = getpgrp();
	int pd[2],fd_in=0;
	int a=0;
	if(strcmp(over_tokens[0],"fg")==0){
		builtInFg(tokens);
		return 1;
	}
	while(over_tokens[a])
	{
		process P;
		tcsetpgrp(shell_terminal,shell_pgid);
		P.argv=(char**)malloc(sizeof(char*)*64);
		resetArgv(P.argv);
		if(pipe_flag){
			pipe(pd);
	  		tokens=getTokens(over_tokens[a],TOKEN_DELIM);
			pname=strdup(tokens[0]);
	  	}
	  	P.pid = fork();
	  	if (P.pid == 0)
		{	
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
		  	signal (SIGTSTP, SIG_DFL);
		  	signal (SIGTTIN, SIG_DFL);
		  	signal (SIGTTOU, SIG_DFL);
		  	signal (SIGCHLD, SIG_DFL);
			if(pipe_flag){
				if(over_tokens[a+1]!=NULL)
					dup2(pd[1],1);
				dup2(fd_in,0);
				close(pd[0]);
			}

			if(check_special_char(tokens,"<")==0)
			{
				if(redir_validator(tokens)==0)
				{
					errorHandle("(file redirection:) Invalid no. of arguments.");
					return 1;
				}
				int p=token_number(tokens,"<");
				if(p==-1)
					return 1;

				int fd=open(tokens[p+1], O_RDONLY,0666);
				if(fd<0)
				{
					perror("fd");
					return 1;
				}
				if(dup2(fd,0)<0)
				{
					perror("dup2");
					return 1;
				}
				close(fd);
				int i,j=0;
				for(i=0;tokens[i];i++){
					if((i==p || i==p+1))
						continue;			
					P.argv[j++]=tokens[i];
				}
				riFlag=1;
			}			

			if(riFlag==1){
				for(i=0;P.argv[i];i++){
					tokens[i]=P.argv[i];
				}
				tokens[i]=NULL;
			}

			if(check_special_char(tokens,">>")==0)
			{
				if(redir_validator(tokens)==0)
				{
					errorHandle("(file redirection:) Invalid no. of arguments.");
					return 1;
				}
				int p=token_number(tokens,">>");
				if(p==-1)
					return 1;

				int fd=open(tokens[p+1], O_APPEND |O_WRONLY | O_CREAT,0666);
				if(fd<0)
				{
					perror("fd");
					return 1;
				}
				if(dup2(fd,1)<0)
				{
					perror("dup2");
					return 1;
				}
				close(fd);
				int i;
				for(i=0;tokens[i];i++){
					if(strcmp(tokens[i],">>")==0)
						break;
					P.argv[i]=tokens[i];
				}
				P.argv[i]=NULL;
				roFlag=1;
			}

			if(check_special_char(tokens,">")==0)
			{
				if(redir_validator(tokens)==0)
				{
					errorHandle("(file redirection:) Invalid no. of arguments.");
					return 1;
				}
				int p=token_number(tokens,">");
				if(p==-1)
					return 1;
				
				int fd=open(tokens[p+1], O_TRUNC |O_WRONLY | O_CREAT,0666);
				if(fd<0)
				{
					perror("fd");
					return 1;
				}
				if(dup2(fd,1)<0)
				{
					perror("dup2");
					return 1;
				}
				close(fd);
				int i;

				for(i=0;tokens[i];i++){
					if(strcmp(tokens[i],">")==0)
						break;
					P.argv[i]=tokens[i];
				}
				P.argv[i]=NULL;
				roFlag=1;
			}

			if(roFlag==0 && riFlag==0)
			{
				for(i=0;tokens[i];i++)
				{
					if(waitFlag==0 && strcmp(tokens[i],"&")==0)
						break;
					P.argv[i]=tokens[i];
				}
			}

			if(strcmp(tokens[0],"echo")==0){
				builtInEcho(P.argv);
				exit(1);
			}
			else if(strcmp(tokens[0],"jobs")==0){
				builtInJobs(P.argv);
				exit(1);
			}
			else if(strcmp(tokens[0],"kjob")==0){
				builtInKJob(P.argv);
				exit(1);
			}
			else if(strcmp(tokens[0],"overkill")==0){
				builtInOverkill(P.argv);
				exit(1);
			}
			else if(strcmp(tokens[0],"pwd")==0){
				builtInPwd();
				exit(1);
			}
			else if(strcmp(tokens[0],"pinfo")==0){
				builtInPinfo();
				exit(1);
			}
			else if(strcmp(tokens[0],"quit")==0)
			{
				printf("Buh-bye.\n");
				builtInOverkill(P.argv);
				kill(shell_pgid,SIGKILL);
				exit(1);
			}
	   	 	
	   	 	if (execvp(P.argv[0],P.argv)==-1){
	     		perror("aaysh");
	     		exit(1);
	   	 	}
	  	}
	  	else if(P.pid < 0){
	    	perror("aays");
	    	exit(1);
	  	}
	  	else
	  	{
	  		strcpy(pro_name_array[P.pid],pname);
	  		if(waitFlag==1){
	  			to_be_listed=P.pid;
	      		waitpid(P.pid, &(P.status), WUNTRACED);
	  		}
	      	else
	      	{
	      		to_be_listed=P.pid;
	      		P.argv[0]=pname;
	      		setpgid(P.pid,P.pid);
	      		add_process(P);
	      		exitedProcess=P.pid;
			}
			if(pipe_flag){
					to_be_listed=P.pid;
	      			//printf("423\n");
	      			close(pd[1]);
	      			fd_in=pd[0];	
	  		}
					
	  	}
	  	a++;
	  	if(pipe_flag==0)
	  		break;
	}

  return 1;

}



int builtInJobs(char **tokens)
{
	node *t=process_list->next;
	while(t!=NULL){
		printf("[%d] %s [%d]\n",t->list_rank,pro_name_array[t->P.pid],t->P.pid);
		t=t->next;
	}
}

int builtInFg(char **tokens)
{
	signal(SIGCHLD, backHandler);
	if(tokens[1]==NULL){
		errorHandle("No job mentioned.");
		return 1;
	}
	process P;
	P.pid=atoi(tokens[1]);
	to_be_listed=P.pid;
	setpgid(P.pid,shell_pgid);
	tcsetpgrp(shell_terminal,P.pid);
	delete_process(P.pid,SIGCONT);
	waitpid(P.pid, &(P.status), WUNTRACED);
	tcsetpgrp(shell_terminal,shell_pgid);
	return 1;
}

int builtInKJob(char **tokens)
{
	if(tokens[1]==NULL)
		errorHandle("No job mentioned.");
	else if(tokens[2]==NULL){
		delete_process(atoi(tokens[1]),9);
	}
	else
		delete_process(atoi(tokens[1]),atoi(tokens[2]));
	return 1;
}

int builtInOverkill(char **tokens)
{
	node *t=process_list->next;
	while(t!=NULL){
			free(t);
			kill(t->P.pid,SIGKILL);
		t=t->next;
	}
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

int token_count(char **tokens)
{
	int c=0;
	while(tokens[c]!=NULL)
		c++;
	return c;

}

int shellExecution(char **tokens)
{
	if(tokens[0]!=NULL)
		return processLaunch(0,tokens);
	return 1;
}

int pipedExecution(char **tokens,int no_of_pipes)
{
	int i;
	for(i=0;tokens[i];i++)
		if(getTokens(tokens[i],TOKEN_DELIM)[0]==NULL){
			errorHandle("Invalid or null tokens between pipes.");
			return 1;
		}
	if(tokens[no_of_pipes]!=NULL)
		return processLaunch(1,tokens);
	else
		perror("pipe");
	return 1;
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
		char **pipetokens=malloc(sizeof(char*)*buffSize);
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
			int f=0,k,pipecount=0;
			//printf("%s\n",tokenoftokens[i]);
			for(k=0;tokenoftokens[i][k];k++)
				if(tokenoftokens[i][k]=='|')
					{f=1;pipecount++;}
			if(f==1)
			{
				pipetokens=getTokens(tokenoftokens[i],"|");
				flag=pipedExecution(pipetokens,pipecount);
			}
			else{
				tokens=getTokens(tokenoftokens[i],TOKEN_DELIM);
				flag=shellExecution(tokens);
			}
			int j=0;
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
  	process_list=(node*)malloc(sizeof(node));
  	process_list->next=NULL;
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