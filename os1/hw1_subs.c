/*
 * hw1_subs.c
 *
 *  Created on: 16 бреб„ 2017
 *      Author: Eden
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc,char** argv)
{
	char* HW1DIR=getenv("HW1DIR"); //get environment variable named HW1DIR
 	char* HW1TF=getenv("HW1TF"); //get environment variable named HW1TF
	if(!HW1DIR || !HW1TF) return 1;
	char* inputpath=(char*) malloc(sizeof(HW1DIR)+sizeof(HW1TF)+2);
	strcpy(inputpath,HW1DIR); //concatenate strings into input file path
	strcat(inputpath,"\\");
	strcat(inputpath,HW1TF);
	int fd=open(inputpath,O_RDWR); //open file by system call
	free(inputpath);
	free(HW1DIR);
	free(HW1TF);
	off_t file_length= lseek(fd,0,SEEK_END); //find length of file
	char* buffer=(char*) malloc(file_length);
	int bytes_read=read(0,buffer,file_length); // read file into buffer
	if(bytes_read<file_length) return 1; //file read error
	if(replace(buffer,argv[1],argv[2])==1) return 1; //replace string contained in argv[1] by string contined in argv[2]
	bytes_read=write(fd,buffer,file_length); //write buffer to file
	if(bytes_read!=file_length) return 1; //write error
	free(buffer);
	close(fd); //free all resources
	return 0;
}

/*
 * Given a text text and 2 strings string_to_replace and replacement,
 * replaces all occurences of string_to_replace in text by replacement
 */
int replace(char* text,char* string_to_replace, char* replacement){
    if(!text || !string_to_replace || !replacement) return 1;
	char* buffer=(char*) malloc (sizeof(text));
    if(!buffer) return 1;
    char* ins = text;
    while((ins=strstr(ins, string_to_replace))){ //find next instance of substring to replace
        strncpy(buffer,	text, ins-text); //copy unchanged part
        buffer[ins-text] = '\0'; //strcat assumes string contains '\0'
        strcat(buffer, replacement); //concatenate replacement string to unchanged part
        /*strcat(buffer, ins+strlen(orig_string));
        strcpy(string_to_replace, buffer);*/
        ins++; //increase pointer in order to avoid inifnite loop
    }
    free(buffer);
    free(ins); //free all resources
    return 0;
}
