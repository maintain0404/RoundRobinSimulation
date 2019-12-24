#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include <errno.h>
#include "logger.c"

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#define PAGE_SIZE 8
//페이지 4개

typedef struct mmanger{
	List * dlist;
	int * memory;
}m_manager;
m_manager manager;

void memory_manager_init(){
	int i;
	manager.dlist = NewList();
	manager.memory = (int*)malloc(sizeof(int) * PAGE_SIZE);
	for(i = 0; i < PAGE_SIZE; i++){
		AddData(manager.dlist, i);
		*(manager.memory + i) = -1;
	}
	//유효하지 않은 임시 데이터 채워 넣음
}

void memory_alloc(int pid){
	int target;
	for(target = 0; target < PAGE_SIZE; target++){
		if(*(manager.memory + target) == pid){
			log_info(BOTH,"pid : %d, page hit\n",pid);
			return;
		}
	}
	target = popData(manager.dlist);
	AddData(manager.dlist, pid);
	*(manager.memory + target) = pid;
	log_info(BOTH,"pid : %d page fault\n", pid);
}

void end_memory_manager(){
	free(manager.memory);
	DeleteList(manager.dlist);
}

#endif