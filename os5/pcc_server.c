/*
 * pcc_server.c
 *
 *  Created on: 16 בינו׳ 2018
 *      Author: Eden
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>


#define MAX_CLIENTS 15 //max. number of pending clients

//this is for convenience
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

//DATA STRUCTURE
int pcc_total[94];

//THREAD-RELATED VARIABLES
pthread_mutex_t lock;
pthread_cond_t thread_conds[MAX_CLIENTS];
int active_threads=0;
int sockfd;



//-----------SEND AND RECV MODIFIED TO READ ENTIRE STRING
int sendall(int sockfd,char* message)
{
	int bytes_sent=0;
	int bytes_left=strlen(message);
	int n=0;
	while(bytes_sent<strlen(message))
	{
		n=send(sockfd,message+bytes_sent,bytes_left,0);
		if(n==-1)
		{
			printf("Error in system call: %s\n",strerror(errno));
			free(message);
			return -1;
		}
		bytes_sent+=n;
		bytes_left-=n;
	}
	return 0;
}

int recvall(int sockfd,char* message)
{
	int bytes_read=recv(sockfd,message,1,0);
	int total=1;
	//printf("recieved bit\n");
	char* temp;
	while(bytes_read!=0)
	{
		temp=message;
		message=realloc(message,strlen(message)+1);
		//printf("realloc'd\n");
		if(!message)
		{
			free(temp);
			return -1;
		}
		bytes_read=recv(sockfd,message+total,1,MSG_DONTWAIT);
		total++;
		//printf("recieved byte!\n");
		if(bytes_read<0)
		{
			if(errno==EAGAIN || errno==EWOULDBLOCK)
				break;
			printf("Error in system call: %s\n",strerror(errno));
			return -1;
		}
	}
	//printf("recieved all\n");
	return 0;
}


int MessageSend(int sockfd,char* message)
{
	if (sendall(sockfd,message)== 1)
		return 1;
	else
		return -1;
}

int MessageRecv(int sockfd,char* message){
	if (recvall(sockfd,message) == 1)
		return 1;
	else
		return -1;
}

////------------SERVER INIT FUNCTION--------------
int ServerInit(int sockfd,sockaddr_in server_addr,int port)
{
	bzero((char*) &server_addr,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=INADDR_ANY;
	server_addr.sin_port=htons(port);
	//memset(server_addr.sin_zero,'\0',sizeof(server_addr.sin_zero));
	if(bind(sockfd,(struct sockaddr*)  &server_addr,sizeof(server_addr))<0)
	{
		perror("bind fail.Error");
		return 0;
	}
	return 1;

}



///-----------SIGNAL HANDLER FUNCTION---------
void HandleSignal(int signum,siginfo_t* siginfo,void* something)
{
	if(signum==SIGINT)
	{
		while(active_threads>0);
		int i;
		for(i=0;i<94;i++)
		{
			printf("char '%c': %u times\n",i+32,pcc_total[i]);
		}
		close(sockfd);
	}
}


///itoa doesn't seem to work under c99/gnu99
void itoa(int num, char *str)
{
        if(str != NULL)
        {
        	sprintf(str, "%d", num);
        }
}




//------------THREAD FUNCTION--------------
void* thread_func(void* thread_param)
{
	pthread_mutex_lock(&lock);
	active_threads++;
	pthread_mutex_unlock(&lock);
	printf("passed lock\n");
	int sockfd=(intptr_t) thread_param;
	char* message=malloc(1);
	if(MessageRecv(sockfd,message)==-1)
		printf("error:%s\n",strerror(errno));
	//printf("recieved!");
	int count=0;
	int i;
//	printf("message=%s\n",message);
	for(i=0;i<strlen(message);i++)
	{
		if((int)(message[i])>31 && (int)(message[i])<127)
		{
	//		printf("message[i]=%c\n",message[i]);
			count++;
			pthread_mutex_lock(&lock);
			pcc_total[(int)(message[i])-32]++;
			pthread_mutex_unlock(&lock);
		}
	}
	char* countbuffer=malloc(strlen(message));
	itoa(count,countbuffer);
	pthread_mutex_lock(&lock);
	MessageSend(sockfd,countbuffer);
	//printf("sent!\n");
	active_threads--;
	pthread_mutex_unlock(&lock);
	pthread_exit(NULL);
	return 0;
}

int main(int argc,char** argv)
{
	if(argc!=2)
	{
		printf("Invalid arguments.");
		return -1;
	}
	//-----------SERVER INIT------------
	//---------VARIABLES AND DS INIT----------------
	sockfd=socket(AF_INET,SOCK_STREAM,0); //socket opening
	if(sockfd<0)  {
		printf("Error:%s",strerror(errno));
			return -1;
	}
	int sigint_triggered=0;
	sockaddr_in server_addr,client_addr;
	int port=atoi(argv[1]);
	if(ServerInit(sockfd,server_addr,port)==0)
	{
		printf("Error in system call:server init fail\n");
		return -1;
	}
	if (listen(sockfd,MAX_CLIENTS)==-1)
	{
		perror("listen fail. Error");
		return -1;
	}

	if(pthread_mutex_init(&lock,NULL)!=0)
	{
		perror("mutex init failed \n");
				return -1;
	}

	socklen_t client_len;
	int client_sockfd;
	//----------------SERVER SIGNAL HANDELR-------------
	struct sigaction sig_handler; //signal handler struct
	//memset(&sig_handler,0,sizeof(sig_handler));
	sigemptyset(&sig_handler.sa_mask);
	sig_handler.sa_sigaction=HandleSignal;
	sig_handler.sa_flags=SA_SIGINFO;
	if(sigaction(SIGINT,&sig_handler,0)!=0)
	{
		printf("Signal error:%s\n",strerror(errno));
	}
	//-----------------SERVER LOOP------------------
	while(!sigint_triggered)
	{
		client_len=sizeof(client_addr);
		client_sockfd= accept(sockfd,(sockaddr*) &client_addr,&client_len);
		//printf("accepted!\n");
		if(client_sockfd==-1)
		{
			printf("Error:%s\n",strerror(errno));
			break;
		}
		pthread_t thread;
		int check=pthread_create(&thread,NULL,thread_func,(void*)(intptr_t)client_sockfd);
		if(check)
		{
			perror("error in creating thread:");
			return -1;
		}
	}


}
