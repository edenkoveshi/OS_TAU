/*
 * myshell.c
 *
 *  Created on: 30 бреб„ 2017
 *      Author: Eden
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

void HandleSignal(int signum,siginfo_t* siginfo,void* whatever)
{
	if(signum==SIGINT)
	{
		kill(siginfo->si_pid,SIGKILL);
	}
/*	if(signum==SIGCHLD)
	{
		kill(siginfo->si_pid,SIGKILL);
	}*/
}

int process_arglist(int count, char** arglist)
{
	int pid; //child process id
	int pid2=-1; //second child process id - in case of pipe
	int i; //temp. var. for iterations
	int pipeflag=0; //binary indicating whether command is pipe command or not
	int pfds[2]; //pipe file descriptors
	int j=0; //pipe location
	int bg=0; //background process
	int status; //child process status upon termination
	struct sigaction sig_handler; //signal handler struct
	memset(&sig_handler,0,sizeof(sig_handler));
	sig_handler.sa_sigaction=HandleSignal;
	sig_handler.sa_flags=SA_SIGINFO;

	//looking for background command
	if(!strcmp(arglist[count-1],"&")) // & found
	{
		arglist[count-1]=0; //remove ampersand
		bg=1;
	}
	//looking for pipe command
	if(!bg) //assuming pipes and background procs. won't be combined
	{
		for(i=0;i<count;i++) //assuming only 1 pipe is valid
		{
			if(!strcmp(arglist[i],"|")) //found
				{
					pipeflag=1;
					j=i;
				}
		}
	}
	if(pipeflag)
	{
			arglist[j]=0; //remove pipe symbol
	}

	pid=fork();
	if(pipeflag) pipe(pfds);

	if(pid<0) //error
	{
		printf("fork error:%s\n",strerror(errno));
		return 0;;
	}

	if(pid==0) //child
	{
		if((!bg && sigaction(SIGINT,&sig_handler,0)!=0))// || sigaction(SIGCHLD,&sig_handler,0)!=0 )
		{
			printf("Signal error:%s\n",strerror(errno));
		}
		if(bg)
		{ //background processes ignore ctrl-c
			struct sigaction bg_sig_handler;
			memset(&bg_sig_handler,0,sizeof(bg_sig_handler));
			bg_sig_handler.sa_handler=SIG_IGN;
			bg_sig_handler.sa_flags=SA_SIGINFO;
			sigaction(SIGINT,&sig_handler,0);
		}
		if(!pipeflag)
		{
			if(execvp(arglist[0],arglist)==-1)
			{
				printf("error:%s\n",strerror(errno));
				exit(EXIT_FAILURE);
			}
		}
		else //pipe
		{
			pid2=fork();

			if(pid2<0) //error
			{
				printf("fork error:%s\n",strerror(errno));
				exit(EXIT_FAILURE);
			}

			if(pid2==0) //second child
			{

				if(sigaction(SIGINT,&sig_handler,0)!=0)
				{
					printf("Signal error:%s\n",strerror(errno));
				}
				close(pfds[0]); //close read end of pipe
				dup2(pfds[1],1); //redirect STDOUT to write end of pipe
				if(execvp(arglist[0],arglist)==-1)
				{
					printf("error:%s\n",strerror(errno));
					exit(EXIT_FAILURE);
				}
			}
			else //first child
			{
				close(pfds[1]); //close write end of pipe
				dup2(pfds[0],0); //redirect read end of pipe to STDIN
				if(execvp(arglist[j+1],arglist+j+1)==-1)
				{
					printf("error:%s\n",strerror(errno));
					exit(EXIT_FAILURE);
				}
				/**
				 * A little explanation - proc1 forks and creates proc2.
				 * proc2 redirects it's STDOUT to the pipe connecting proc1 and 2.
				 * proc2 executes command and output is sent through pipe
				 * meanwhile, proc1 redirects it's read end of pipe to STDIN,
				 * and executes second command with input read through pipe.
				 */
			}
		}
	}

	else //parent
	{
		struct sigaction parent_sig_handler;
		memset(&parent_sig_handler,0,sizeof(parent_sig_handler));
		parent_sig_handler.sa_handler=SIG_IGN;
		parent_sig_handler.sa_flags=SA_SIGINFO;

		if(sigaction(SIGINT,&parent_sig_handler,0)!=0)
		{
			printf("Signal error:%s\n",strerror(errno));
		}
		if(!bg)
		{
			while(-1!=waitpid(0,&status,0))
			{
				if(status==-1)
					kill(pid,SIGKILL);
			}
			//wait until all children terminate
		}
	}
	return 1;
}

int prepare(void) {
	return 0;
}
int finalize(void) {
	int status;
	while(-1!=waitpid(-1,&status,WNOHANG));
	return 0;}
