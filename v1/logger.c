#include<stdio.h>
#include<time.h>
#include<unistd.h>
#include<errno.h>

#ifndef LOGGER_H
#define LOGGER_H

#define BOTH 0
#define FILE_ONLY 1
#define STDOUT_ONLY -1

typedef struct __log{
	FILE * file;
	int option;
}log;

char * get_ctime(){
	time_t t = time(NULL);
	char * ctime;
	ctime = (char*)malloc(sizeof(char) * 30);
	struct tm crt_time = *localtime(&t);
	sprintf(ctime, "%d-%d-%d %d:%d:%d",
		   crt_time.tm_year + 1900 , crt_time.tm_mon + 1, crt_time.tm_mday,
		   crt_time.tm_hour, crt_time.tm_min, crt_time.tm_sec);
	return ctime;
}

FILE* log_init(){
	char filename[100];
	FILE *fp;
	
	time_t t = time(NULL);	
	struct tm crt_time = *localtime(&t);
	sprintf(filename, "%d-%d-%d %d:%d:%d.txt",
		   crt_time.tm_year + 1900 , crt_time.tm_mon + 1, crt_time.tm_mday,
		   crt_time.tm_hour, crt_time.tm_min, crt_time.tm_sec);
	//현재시각으로 파일명 설정
	
	fp = fopen(filename, "w");
	setvbuf(fp, NULL, _IOFBF, 1024);
	//버퍼 설정
	return fp;
}

void log_info(FILE* fp, char* msg, int option){
	char * ctime = get_ctime();
	pid_t pid = getpid();
	if(option <= 0){
		printf("INFO pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	if(option >= 0){
		fprintf(fp, "INFO pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	free(ctime);
}

void log_debug(FILE* fp, char* msg, int option){
	char * ctime = get_ctime();
	pid_t pid = getpid();
	if(option <= 0){
		printf("DEBUG pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	if(option >= 0){
		fprintf(fp, "DEBUG pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	free(ctime);
}

void log_error(FILE* fp, char* msg, int option){
	char * ctime = get_ctime();
	pid_t pid = getpid();
	if(option <= 0){
		printf("ERROR pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	if(option >= 0){
		fprintf(fp, "ERROR pid:%d time:%s\n%s\n", pid, ctime, msg);
	}
	free(ctime);
}
			   
#endif
