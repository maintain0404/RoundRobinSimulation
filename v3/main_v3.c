#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include<signal.h>
#include<string.h>
#include<sys/msg.h>
#include<sys/wait.h>
#include "process.c"
#include <errno.h>
#include "queue.c"
#include "logger.c"

#define MSGQ_KEY 1111
#define QUANTUM 50
#define PROCESS_COUNT 10
#define QUANTUM_COUNT 1000

typedef struct __msg{
	long mtype;
	prs_info pinfo;
}message;

prs_info * processes[PROCESS_COUNT];
key_t msgq_id;
FILE * log_file;

void child_process(int pint, prs_info * prs, key_t id){
	int i = 0;
	int work_state;
	char log_msg[256];
	message temp;
	prs = processes[pint - 1];
	prs->pid = getpid();
	
	//자식 시작 완료 메시지 보내기
	temp.mtype = pint;
	if(msgsnd(id, &temp, sizeof(prs_info), 0) != -1){
		sprintf(log_msg, "child process id : %d On\n", prs->pid);
		log_info(log_file, log_msg, BOTH);	
	}else{
		sprintf(log_msg, "child process failed ERROR : %s\n", strerror(errno));	
		log_error(log_file, log_msg, BOTH);
		exit(-1);
	}
	
	
	//본격적 작업처리 부분
	while(1){
		if(msgrcv(id, &temp, sizeof(prs_info), prs->pid, 0) != -1){
		//메시지 받았을 떄 조건
			work_state = work_process(prs, QUANTUM);
			//작업 할당량이 남았을 때
			if(work_state > 0){
				sprintf(log_msg, "%d process work %d => %d\n", pint, prs->work + QUANTUM, prs->work);
				log_info(log_file, log_msg, BOTH);
			//IO CPU 전환할 때 
			}else if(work_state <= 0){
				sprintf(log_msg, "work change!\n");
				log_info(log_file, log_msg, BOTH);
				temp.pinfo = *prs;
				temp.mtype = getppid();
				msgsnd(id, &temp, sizeof(prs_info), 0);
			}	
		}else{
		//메시지큐 에러
			sprintf(log_msg, "%d process msgq ERROR! : %s\n",pint, strerror(errno));
			log_error(log_file, log_msg, BOTH);
		}
		
	}
	return;
}


//전역변수 사용중, 메시지큐 대체할 여유가 나면 할것
//정보를 읽고 처리만 한다. 메인에서 자식의 수정 요청을 처리하고, 여기서 스케줄링만 한다면?
void handler(int signum){
	static int cnt = 0;
	static Queue Q;
	static Queue priorityQ;
	static Queue waitQ;
	char log_msg[256];
	char msg_bf[128];
	message content;
	int i, work_state, j;
	prs_info * temp;
	
	printf("handle %d\n", cnt);
	//프로세스 핸들링 전처리부분 TODO메인으로 옮기면 좋을듯
	//종료
	if(cnt >= QUANTUM_COUNT){
		exit(-1);
	}
	//초기화
	if(cnt == 0){
		
		sprintf(log_msg, "signal init\n");
		log_info(log_file, log_msg, BOTH);
		InitQueue(&Q);
		InitQueue(&priorityQ);
		InitQueue(&waitQ);
		for(i = 0; i < PROCESS_COUNT; i++){
			Enqueue(&Q, processes[i]);
			cnt++;
		}
	}
	//본격적 처리 부분
	else{
		//자식 IO전환 정보 IPC 메시지 확인 + 정보 갱신
		//TODO 큐 전체를 순환해서 속도 저하가 일언암
		if(msgrcv(msgq_id, &content, sizeof(prs_info), getpid(), IPC_NOWAIT) != -1){
			for(i = 0; i < Q.count; i++){
				temp = Dequeue(&Q);
				if(temp->type == content.pinfo.type){
					Enqueue(&waitQ, temp);	
				}else{
					Enqueue(&Q, temp);
				}
			}
			for(i = 0; i < priorityQ.count; i++){
				temp = Dequeue(&priorityQ);
				if(temp->type == content.pinfo.type){
					Enqueue(&waitQ, temp);	
				}else{
					Enqueue(&priorityQ, temp);
				}
			}
		}
		//IO 부분
		j = waitQ.count;
		memset(msg_bf, 0, sizeof(char) * 128);
		memset(log_msg, 0, sizeof(char) * 256);
		for(i = 0; i < j; i++){
			temp = Dequeue(&waitQ);
			work_state = work_process(temp, QUANTUM);
			sprintf(msg_bf, "waitQ %d run %d -> %d\n", temp->type, temp->work, temp->work - QUANTUM);
			strcat(log_msg, msg_bf);
			if(work_state == 0){
				//우선순위 구분
				if(temp->priority > (PRIORITY_MAX / 4 * 3)){
					Enqueue(&priorityQ, temp);
				}else{
					Enqueue(&Q, temp);
				}			
			}else{
				Enqueue(&waitQ, temp);
			}
		}
		if(strlen(log_msg) != 0){
			log_info(log_file, log_msg, BOTH);
		}
		
		//우선 큐
		if(cnt % 5 != 0){
			temp = Dequeue(&priorityQ);
			if(temp != NULL){
				content.mtype = (long)temp->pid;
				content.pinfo = *temp;
				if(msgsnd(msgq_id, &content, sizeof(prs_info), 0) == -1){
					sprintf(log_msg, "sendMSG failed ERROR : %s\n", strerror(errno));	
					log_error(log_file, log_msg, BOTH);
				}else{
					sprintf(log_msg, "id : %d work : %d PriorityQ SendMSG\n", temp->type, temp->work);
					log_info(log_file, log_msg, STDOUT_ONLY);
				}
				Enqueue(&priorityQ, temp);
				cnt++;
				return;  //우선 큐 처리하면 퀀텀 패쓰
			}		
		}
		//일반 큐
		temp = Dequeue(&Q);
		//모든 큐에 프로세스가 없을 때
		if(temp != NULL){
			content.mtype = (long)temp->pid;
			content.pinfo = *temp;
			if(msgsnd(msgq_id, &content, sizeof(prs_info), 0) == -1){
				sprintf(log_msg, "sendMSG failed ERROR : %s\n", strerror(errno));	
				log_error(log_file, log_msg, BOTH);
			}else{
				sprintf(log_msg, "id : %d work : %d NormalQ SendMSG\n", temp->type, temp->work);
				log_info(log_file, log_msg, STDOUT_ONLY);
			}
			Enqueue(&Q, temp);
		}
	}
	cnt++;
}

int main(){
	int i = 0, j;
	pid_t crt;
	key_t msgkey;
	message content;
	char log_msg[256];
	
	log_file = log_init();
	
	//메시지큐 생성
	msgkey = ftok("/workspace/TermProject", 1);
	msgq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0640);
	if(msgq_id > 0){
		sprintf(log_msg,"messageQ %d created\n", msgq_id);
		log_info(log_file, log_msg, 0);
	}else{
		sprintf(log_msg, "message create failed ERR : %d\n", errno);
		log_error(log_file, log_msg, 0);
		return 0;
	}
	
	
	//1번은 그냥 생성
	do{
		crt = fork();
		if(crt < 0){
			sprintf(log_msg,"%d child process create failed\n", i + 1);
			log_error(log_file, log_msg, 0);
			continue;
		}else{
			processes[i] = mk_process(i + 1);
			if(crt == 0){
				sprintf(log_msg, "%d child process created\n", i + 1);
				log_info(log_file, log_msg, BOTH);
				child_process(i + 1, processes[i], msgq_id);
				break;
			}else{
				processes[i]->pid = crt;
				i++;
			}
		}
	} while(crt != 0 && i < PROCESS_COUNT);
	//여기서부터는 부모 프로세스만 실행됨
	//메시지를 입력하는 부분
	if(crt > 0){
		struct sigaction sa;
		struct itimerval timer;
		
		//타이머 설정
		memset(&sa, 0, sizeof (sa));
		sa.sa_handler = handler;
		sigaction (SIGALRM, &sa, NULL);

		timer.it_value.tv_sec = 3;
		timer.it_value.tv_usec = 0;

		timer.it_interval.tv_sec = QUANTUM / 1000;
		timer.it_interval.tv_usec = QUANTUM * 1000;

		for(i = 0; i < PROCESS_COUNT; i++){
			if(msgrcv(msgq_id, &content, sizeof(prs_info), i + 1, 0) != -1){
				printf("%d process clear!\n", i + 1);
			}else{
				printf("no %d process\n", i + 1);
			}
		}
		sprintf(log_msg, "all child set ready\n\n\n\n");
		log_info(log_file, log_msg, BOTH);
		//가상 타이머 시작
		if(setitimer(ITIMER_REAL, &timer, NULL) == -1){
			printf("%s\n", strerror(errno));
			printf("timer error\n");
			exit(-1);
		}//TOTO플래그 바꿔야함

		//종료 대기
		while(1){
			if(msgrcv(msgq_id, &content, sizeof(prs_info), 0, IPC_NOWAIT) != -1){
				if(content.pinfo.type == 0){
					break;
				}
			}
			sleep(1);
		};
	}
	return 0;
}
