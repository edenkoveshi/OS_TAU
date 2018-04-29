/*
 * message_reader.c
 *
 *  Created on: 16 בדצמ× 2017
 *      Author: Eden
 */
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAJOR_NUM 244
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

typedef struct student{
	char* name;
	int age;
	char class;
} student;


int main(char** argv,int argc)
{
	if(argc!=3)
	{
		printf("Invalid paramereadters. Please try again.\n");
		return -1;
	}
	int fd=open(argv[1],O_RDONLY);
	if(fd==-1)
	{
		printf("Invalid file path. Please try again.\n");
		return -1;
	}
	int channel=atoi(argv[2]);//it is said we can assume that channel would be passed as a number
	if(ioctl(fd,MSG_SLOT_CHANNEL,channel)!=0) //setting channel has failed
	{
		printf("error:%s\n",strerror(errno));
		return -1;
	}
	char buffer[128];
	if(read(fd,buffer,128)==-1)
	{
		printf("error:%s\n",strerror(errno));
		return -1;
	}
	close(fd);
	printf("Message '%s' has been successfully read from channel %d\n",buffer,channel);
	return 0;
}


