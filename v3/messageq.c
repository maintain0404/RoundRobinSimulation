#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include <errno.h>
#include "logger.c"

#ifndef MESSAGEQ_H
#define MESSAGEQ_H

#define PPRS_INIT 1
//초기 세팅
#define PPRS_END 2
//완전 종료
#define PPRS_CPU_TO_IO 3
//프로세스 작업 전환
#define CPRS_WORK 4
//프로세스 작업

char * Macro[4] = {
	"process(child) init",
	"program end",
	"process(child) changed to IO",
	"process(child) work",
};

typedef struct ___msg{
	long mtype;
	unsigned char msg_type;
	prs_info * pinfo;
}msg_content;

key_t msgq_id;

key_t msq_init(){
	key_t msgkey;
	msgkey = ftok("/workspace/TermProject", 1);
	msgq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0640);
	if(msgq_id > 0){
		log_info(0, "messageQ %d created\n", msgq_id);
	}else{
		log_error(0, "message create failed ERR : %d\n", errno);
		exit(1);
	}
}

void send_msg(long pid_to, unsigned char msg_type, prs_info * pinfo){
	int result;
	msg_content mcnt;
	mcnt.mtype = pid_to;
	mcnt.msg_type = msg_type;
	mcnt.pinfo = pinfo;
	result = msgsnd(msgq_id, &mcnt, sizeof(mcnt.pinfo) + sizeof(unsigned char), IPC_NOWAIT);
	if(result == -1){
		log_error(BOTH,"pid : %d %s send_msg failed : %s\n", pinfo->pid, Macro[msg_type - 1], strerror(errno));
	}else{
		log_info(STDOUT_ONLY, "pid : %d %s send_msg success\n", pinfo->pid, Macro[msg_type - 1]);
	}	
}

prs_info * rcv_msg(long pid_from){
	int result;
	msg_content * mcnt = (msg_content*)malloc(sizeof(msg_content));
	result = msgrcv(msgq_id, mcnt, sizeof(mcnt->pinfo) + sizeof(unsigned char), pid_from, 0);
	if(result == -1){
		log_error(BOTH,"rcv_msg failed : %s\n", strerror(errno));
		free(mcnt);
		return NULL;
	}else{
		log_info(BOTH, "pid : %d %s rcv_msg success\n", pid_from, Macro[mcnt->msg_type - 1]);
		free(mcnt);
		return mcnt->pinfo;
	}
}
msg_content * parent_rcv_msg(long pid_from){
	int result;
	msg_content * mcnt;
	
	result = msgrcv(msgq_id, mcnt, sizeof(prs_info) + sizeof(unsigned char), pid_from, IPC_NOWAIT);
	if(result == -1){
		log_error(BOTH,"rcv_msg failed : %s\n", strerror(errno));
		return NULL;
	}else{
		log_info(BOTH, "pid : %d %s rcv_msg success\n", pid_from, Macro[mcnt->msg_type - 1]);
		return mcnt;
	}
}

#endif