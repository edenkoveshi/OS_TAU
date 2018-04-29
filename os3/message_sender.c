/*
 * message_sender.c
 *
 *  Created on: 15 בדצמ× 2017
 *      Author: Eden
 */
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define MAJOR_NUM 244
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

int main(int argc,char** argv)
{
	if(argc!=4)
	{
		printf("%d\n",argc);
		printf("Invalid parameters. Please try again.\n");
		return -1;
	}
	int fd=open(argv[1],O_WRONLY);
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
	if(write(fd,argv[3],strlen(argv[3]))==-1)
	{
		printf("error:%s\n",strerror(errno));
		return -1;
	}
	close(fd);
	printf("Message '%s' has been successfully sent on channel %d\n",argv[3],channel);
	return 0;
}


