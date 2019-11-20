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

#define MSGQ_KEY 1111
#define QUANTUM 64
#define PROCESS_COUNT 5

typedef struct __msg{
	long mtype;
	char mtext[256];
}message;

prs_info * processes[PROCESS_COUNT];
key_t msgq_id;

void child_process(int pint, prs_info * prs, key_t id){
	int i = 0;
	message temp;
	prs = processes[pint - 1];
	prs->pid = getpid();
	printf("cildprocess On\n");
	printf("id : %d\n", id);
	
	//작업 처리 전 세팅부분
	// temp.mtype = getppid();
	// memcpy(temp.mtext, "Hello World\n", 13);//TODO메시지 바꿀 것
	// msgsnd(id, temp, 64, 0);	
	
	//본격적 작업처리 부분
	while(1){
		if(msgrcv(id, &temp, 64, pint, 0) != -1){
		//메시지 받았을 떄 조건
			printf("%d process get message\n", pint);
			prs->work -= QUANTUM;	//TODO이 prs랑 부모의 prs랑 달라서 생기는 문제인듯				
			if(prs->work > 0){
				printf("%d process work %d => %d\n", pint, prs->work + QUANTUM, prs->work);
			}else{
				//프로세스 종료 로그
				printf("%d process END\n", pint);
				break;
			}	
		}else{
		//메시지큐 에러
			printf("%d process msgq ERROR! : %d\n",pint, errno);
			printf("ERRNO : %s\n", strerror(errno));
		}
		
	}
	return;
	// for(i = 0; i < 60; i++){
	// 	msgrcv(MSGQ_KEY, (void *)&temp, sizeof(message), 0, 0);
	// 	printf("%s\n", temp.msg);
	// 	printf("%d process executing %d times...\n", pint, i + 1);
	// 	sleep(1);
	// }
}


//전역변수 사용중, 나중에 메세지큐로 대체할 것
void handler(int signum){
	static int cnt = 0;
	static Queue Q;
	message content;
	int i;
	prs_info * temp;
	
	//프로세스 핸들링 전처리부분 TODO메인으로 옮기면 좋을듯
	if(cnt == 0){
		printf("signal init\n");
		InitQueue(&Q);
		for(i = 0; i < PROCESS_COUNT; i++){
			Enqueue(&Q, processes[i]);
			cnt++;
		}
	}
	//본격적 처리 부분
	else{
		temp = Dequeue(&Q);
		
		if(temp == NULL){
			printf("All process clear!\n");
			content.mtype = getpid();
			memcpy(content.mtext, "Hello World\n", 13);	//나중에 메시지 수정할 것
			msgsnd(msgq_id, &content, 64, 0);
			return;
		}
		else if(temp->work > 0){
			content.mtype = temp->type;
			memcpy(content.mtext, "Hello World\n", 13);
			if(msgsnd(msgq_id, &content, 64, 0) == -1){
				printf("sendMSG failed ERROR : %d\n", errno);	
				printf("%s\n", strerror(errno));
			}else{
				printf("sendMSG\n");
			}
			temp->work -= QUANTUM;	//TODO 이거 자식에서 처리하게 바꾸야될듯/pdf에는 부모에서 계산하라고함
			Enqueue(&Q, temp);
		}
		else{
			
		}	
	}
}

int main(){
	int i = 0, j;
	pid_t crt;
	key_t msgkey;
	message content;
	
	//메시지큐 생성
	msgkey = ftok("/workspace/TermProject", 1);
	msgq_id = msgget(IPC_PRIVATE, IPC_CREAT | 0640);
	if(msgq_id > 0){
		printf("messageQ %d created\n", msgq_id);
	}else{
		printf("message create failed ERR : %d\n", errno);
		return;
	}
	
	
	//1번은 그냥 생성
	do{
		crt = fork();
		if(crt < 0){
			printf("%d child process create failed\n", i + 1);
			continue;
		}else{
			processes[i] = mk_process(i + 1);
			if(crt == 0){
				printf("%d child process created\n", i + 1);
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
		
		printf("pprocess\n");
		//타이머 설정
		memset(&sa, 0, sizeof (sa));
		sa.sa_handler = handler;
		sigaction (SIGALRM, &sa, NULL);

		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 250000;

		timer.it_interval.tv_sec = QUANTUM / 1000;
		timer.it_interval.tv_usec = QUANTUM * 1000;

		//가상 타이머 시작
		setitimer (ITIMER_REAL, &timer, NULL);	//TOTO플래그 바꿔야함

		// for(i = 0; i < 10; i++){
		// 	content.mtype = i + 1;
		// 	memcpy(content.mtext, "Hello World\n", 13);
		// 	if(msgsnd(msgq_id, &content, 64, 0) == -1){
		// 		printf("sendMSG failed ERROR : %d\n", errno);
				
		// 		printf("%s\n", strerror(errno));
		// 	}else{
		// 		printf("sendMSG\n");
		// 	}	
		// }
		// for(i = 0; i < 10; i++){
		// 	waitpid(processes[i]->pid, NULL, 0);
		// }	
		while(1){
			if(msgrcv(msgq_id, &content, 64, getpid(), IPC_NOWAIT) != -1){
				break;
			}
			sleep(1);
		};
		printf("END!\n");
	}
	return 0;
}