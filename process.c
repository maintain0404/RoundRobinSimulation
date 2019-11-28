#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>

#ifndef PROCESS_H
#define PROCESS_H

#define WTYPE_CPU 0
#define WTYPE_IO 1
#define W_MIN 50
#define W_RANGE 2000


typedef struct process_{
	pid_t pid;
	int	type;
	int priority;
	int work;
	int wtype;
}prs_info;

prs_info * mk_process(int pint){
	prs_info * temp;
	temp = malloc(sizeof(prs_info));
	
	temp->wtype = WTYPE_CPU;
	temp->type = pint;
	srand(time(NULL));
	temp->priority = rand() % 128;
	srand(time(NULL));
	temp->work = rand() % W_RANGE + W_MIN;
	return temp;
}

//실제 일을 하는 함수 + 끝나면 재할당
int work_process(prs_info * prs, int workamount){
	prs->work -= workamount;
	if(prs->work < 0){
		srand(time(NULL));
		prs->work = rand() % W_RANGE + W_MIN;	
		if(prs->wtype == WTYPE_CPU){
			prs->wtype = WTYPE_IO;
		}else{
			prs->wtype = WTYPE_CPU;	
		}
		return 0;
	}
	return prs->work;
}

//프로세스 삭제함수
void * del_process(prs_info * prs){
	free(prs);
}
#endif

