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
#include "double_linkedlist.c"
#include "memeory_manager.c"
#include "messageq.c"

#define MSGQ_KEY 1111
#define QUANTUM 200
#define PROCESS_COUNT 10
#define QUANTUM_COUNT 1000

typedef struct __msg{
	long mtype;
	prs_info pinfo;
}message;

prs_info * processes[PROCESS_COUNT];
key_t msgq_id;
FILE * log_file;

void end_all(){
	int i;
	for(i = 0; i < PROCESS_COUNT; i++){
		del_process(processes[i]);
		end_memory_manager();
		log_info(BOTH, "END!\n");
	}
}

void child_process(int pint, prs_info * prs, key_t id){
	int work_state;
	prs = processes[pint - 1];
	prs->pid = getpid();
	
	//자식 시작 완료 메시지 보내기
	send_msg(getppid(), PPRS_INIT, prs);
		
	//본격적 작업처리 부분
	while(1){
		prs = rcv_msg((long)(prs->pid));
		//자식에게 오는 건 무조건 노동명령
		work_state = work_process(prs, QUANTUM);
		//작업 할당량이 남았을 때
		if(work_state > 0){
			log_info(BOTH, "%d process work %d => %d\n", pint, prs->work + QUANTUM, prs->work);
		//IO CPU 전환할 때 
		}else if(work_state <= 0){
			send_msg((long)getppid(), PPRS_CPU_TO_IO, prs);
		}
	}
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
		end_all();
		exit(-1);
	}
	//초기화
	if(cnt == 0){
		
		log_info(BOTH, "signal init\n");
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
			log_info(BOTH , log_msg);
		}
		
		//우선 큐
		if(cnt % 5 != 0){
			temp = Dequeue(&priorityQ);
			if(temp != NULL){
				memory_alloc(temp->pid);
				printf("%d\n", temp->pid);
				send_msg((long)(temp->pid), CPRS_WORK, temp);
				Enqueue(&priorityQ, temp);
				cnt++;
				return;  //우선 큐 처리하면 퀀텀 패쓰
			}		
		}
		//일반 큐
		temp = Dequeue(&Q);
		//모든 큐에 프로세스가 없을 때
		if(temp != NULL){
			memory_alloc(temp->pid);
			printf("%d\n", temp->pid);
				
			send_msg((long)(temp->pid), CPRS_WORK, temp);
			Enqueue(&priorityQ, temp);
		}
	}
	cnt++;
}

int main(){
	int i = 0, j;
	pid_t crt;
	message content;
	char log_msg[256];
	
	log_file = log_init();
	memory_manager_init();
	
	//메시지큐 생성
	msq_init();

	//1번은 그냥 생성
	do{
		crt = fork();
		if(crt < 0){
			log_error(0, "%d child process create failed\n", i + 1);
			continue;
		}else{
			processes[i] = mk_process(i + 1);
			if(crt == 0){
				log_info(BOTH, "%d child process created\n", i + 1);
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
			printf("%d", i);
			rcv_msg(getpid());
		}
		log_info(BOTH, "all child set ready\n\n\n\n");
		//가상 타이머 시작
		if(setitimer(ITIMER_REAL, &timer, NULL) == -1){
			printf("%s\n", strerror(errno));
			printf("timer error\n");
			exit(-1);
		}//TOTO플래그 바꿔야함

		//종료 대기
		while(1){
			sleep(1);
		};
	}
	return 0;
}


