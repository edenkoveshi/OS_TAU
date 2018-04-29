/*
 * hw4.c
 *
 *  Created on: 1 בינו׳ 2018
 *      Author: Eden
 */

///------ INCLUDES & DEFINES
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define CHUNK_SIZE 1024*1024
#define SUCCESS 0

///----- GLOBAL VARIABLES -----
pthread_mutex_t thread_lock;
pthread_mutex_t main_lock;
pthread_cond_t write_cond;
pthread_cond_t main_cond;
char* shared_buffer;
int outputfile;
int writes_count=0;
int thread_count=0;
int max_length_to_write=0;

// ------ THREAD FUNCTION ----------
void* thread_func(void* thread_param)
{
	int fd;
	char* filepath=(char*) thread_param;
	//assuming file is open
	char chunk[CHUNK_SIZE];
	int bytes_read;
	int bytes_written;
	int bytes_left;
	int i;
	fd=open(filepath,O_RDONLY);
	while(thread_count>0)
	{
		bytes_read=0;
		bytes_left=CHUNK_SIZE;

		/// ----- CHUNK READ ---------
		while(bytes_left>0) //inifinite loop can't happen, thread will exit if nothing is read
		{
			bytes_read=read(fd,chunk,bytes_left);
			if(bytes_read>max_length_to_write)
				max_length_to_write=bytes_read;
			if(bytes_read<0)
			{
				printf("read error:%s,exiting.\n",strerror(errno));
				pthread_cond_signal(&main_cond);
				exit(-1); //exit ALL threads
			}
			bytes_left=bytes_left-bytes_read;
			if(bytes_read==0&&bytes_left==CHUNK_SIZE) //thread finished reading
			{
				thread_count--;
				close(fd);
				if(thread_count==0)// last thread, free memory and wake up main
				{
					free(shared_buffer);
					pthread_cond_signal(&main_cond);
				}
				pthread_exit(NULL);
			}
			else if(bytes_read==0) //thread partially read a chunk, it will submit it and exit next iteration
			{
				break;
			}
		}

		//------- FILE XOR -----------
		pthread_mutex_lock(&thread_lock);
		i=0;
		while(chunk[i]!='\0')
		{
			shared_buffer[i]=shared_buffer[i]^chunk[i];
			i++;
		}
		writes_count++;
		pthread_mutex_unlock(&thread_lock);

		// ------ SHARED BUFFER COMMIT -----------
		if(writes_count!=thread_count) //not all threads have written
		{
			pthread_mutex_lock(&thread_lock);
			pthread_cond_wait(&write_cond,&thread_lock);
		}
		if(writes_count==thread_count)
		{
			bytes_left=max_length_to_write;
			while(bytes_left>0)
			{
				bytes_written=write(outputfile,shared_buffer,bytes_left);
				lseek(outputfile,0,SEEK_END);
				bytes_left=bytes_left-bytes_written;
				if(bytes_written<=0)
				{
					perror("write error:");
					pthread_cond_broadcast(&write_cond);
					pthread_cond_signal(&main_cond);
					exit(-1); //exit ALL threads
				}
			}

			// ---- PREPARE FOR NEXT ITERATION ------------
			memset(shared_buffer,0,CHUNK_SIZE); //empty buffer
			writes_count=0; //init writes count
			max_length_to_write=0; //init max read data length
			pthread_cond_broadcast(&write_cond); //wake up all sleeping threads
		}
		pthread_mutex_unlock(&thread_lock);
	}
	return SUCCESS;
}

/// --------- MAIN ---------
int main(int argc,char** argv)
{
	//int* fds;
	char* tmp;
	int check;
	struct stat fileStat;

	//----COUNT FILES----
	int i=2;
	thread_count=argc-2;

	//--- INIT GLOBAL VARIABLES ---
	writes_count=0;
	outputfile=open(argv[1],O_TRUNC|O_WRONLY|O_CREAT,0777);
	printf("Hello, creating %s from %d input files\n",argv[1],thread_count);
	//LOCK INIT
	if(pthread_mutex_init(&thread_lock,NULL)!=0 || pthread_mutex_init(&main_lock,NULL)!=0){
		perror("mutex init failed \n");
		return -1;
	}

	//COND INIT
	if(pthread_cond_init(&write_cond,NULL)!=0 || pthread_cond_init(&main_cond,NULL)!=0)
	{
		perror("condition variable init failed \n");
		return -1;
	}

	//--- SHARED BUFFER INIT ---
	shared_buffer=malloc(CHUNK_SIZE);

	//---- THREADS INIT -----
	pthread_t* threads;
	threads=malloc(thread_count*sizeof(pthread_t));
	i=0;
	for(;i<thread_count;i++)
	{
		tmp=argv[i+2];
		check=pthread_create(&threads[i],NULL,thread_func,(void*)tmp);
		if(check)
		{
			perror("error in creating thread:");
			return -1;
		}
	}

	// ---WHEN ALL THREADS ARE FINISHED, EXIT ----
	pthread_mutex_lock(&main_lock);
	pthread_cond_wait(&main_cond,&main_lock);
	if(fstat(outputfile,&fileStat)<0)
	{
		perror("fstat error:");
		return -1;
	}
	printf("Created %s with size %d bytes\n",argv[1],(int)fileStat.st_size);
	close(outputfile);
	pthread_cond_destroy(&main_cond);
	pthread_cond_destroy(&write_cond);
	pthread_mutex_destroy(&main_lock);
	pthread_mutex_destroy(&thread_lock);
	free(threads);
	//free(fds);
	return SUCCESS;
}

