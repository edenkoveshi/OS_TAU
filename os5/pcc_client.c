

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>


//for convenience
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct addrinfo addrinfo;


/////-------------SEND&RECV MODIFIED TO NOT READ/WRITE PARTIAL DATA------------------
int sendall(int sockfd,char message[])
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


int MessageSend(int sockfd,char message[])
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



////Client management
int main(int argc, char** argv){
	if(argc!=4)
	{
		printf("Error: invalid number of parameters");
		return -1;
	}

	//----------SET CONNECTION----------
	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)  {
		printf("Error in system call: %s\n",strerror(errno));
		return 0;
	}
	addrinfo hints,*hostinfo;
	memset(&hints,0,sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;
	int y;
	if((y=getaddrinfo( NULL , "http" , &hints , &hostinfo))!=0)
	{
		printf("%s\n",gai_strerror(y));
		return -1;
	}
	char hostname[256]; //ip
	sockaddr_in* serv_addr=(sockaddr_in*) (hostinfo->ai_addr);
	getnameinfo((sockaddr*) serv_addr, hostinfo->ai_addrlen, hostname, sizeof(hostname), NULL, 0, NI_NUMERICHOST);
	if(inet_aton(hostname,&serv_addr->sin_addr)==0)
	{
		printf("error.%s\n",strerror(errno));
		return -1;
	}

	serv_addr->sin_family=AF_INET;
	int port=atoi(argv[2]);
	serv_addr->sin_port=htons(port);
	memset(serv_addr->sin_zero,0,sizeof(*(serv_addr->sin_zero)));
	if(connect(sockfd,(sockaddr*) serv_addr, sizeof(*serv_addr))<0)
	{
		printf("connection error:%s\n",strerror(errno));
		return -1;
	}

	//connection is now set
	//-----------OPEN RANDOM DEV. AND SEND DATA TO SERVER
	int fdrandom=open("/dev/urandom",O_RDONLY);
	if(fdrandom<0)
	{
		printf("error in system call:%s\n",strerror(errno));
		return -1;
	}
	int length=atoi(argv[3]);
	char msg[length];
	int bytes_read;
	int bytes_left=length;
	int x=read(fdrandom,&msg,length);
	bytes_read=x;
	while(bytes_read<length)
	{
		if(x<0)
		{
			printf("read error: %s\n",strerror(errno));
		}
		bytes_read=bytes_read+x;
		bytes_left=bytes_left-bytes_read;
		x=read(fdrandom,&msg,bytes_left);
	}
	MessageSend(sockfd,msg);
	//printf("sent!\n");
	char* response=malloc(1);
	MessageRecv(sockfd,response);
	//printf("recieved!\n");
	printf("# of printable characters: %u\n",atoi(response));
	return 0;
}




