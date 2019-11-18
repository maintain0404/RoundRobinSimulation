#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>

#ifndef PROCESS_H
#define PROCESS_H

typedef struct process_{
	pid_t pid;
	int	type;
	int priority;
	int work;
}prs_info;

prs_info * mk_process(int pint){
	prs_info * temp;
	temp = malloc(sizeof(prs_info));
	srand(time(NULL));
	
	temp->type = pint;
	temp->priority = rand() % 128;
	temp->work = rand() % 8096 + 8096;
	return temp;
}

#endif

