#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
 
typedef int Element;
 
typedef struct Node{
    Element data;//데이터
	struct Node *next;//링크(다음 노드의 위치 정보)
    struct Node *prev;//링크(이전 노드의 위치 정보)
}Node;
 
 
typedef struct List{
    Node *head;
    Node *tail;
    int count;
}List;
 

 
List *NewList();
void InitList(List *list);
void DeleteList(List *list);
void AddData(List *list, int pid);
void Remove(List *list, Node *now);
Node *Find(List *list, int pid);
void Clear(List *list);
 
 
Node *NewNode(int pid){
	Node *now = (Node *)malloc(sizeof(Node));
    now->data = pid;
    now->prev = now->next = NULL;
    return now;
}
List *NewList(){
    List *list = (List *)malloc(sizeof(List));
    InitList(list);//리스트 초기화
    return list;
}

void DeleteList(List *list){
    Clear(list);
    free(list->head);
    free(list->tail);
    free(list);
}

void InitList(List *list){
    list->head = NewNode(0);//연결 리스트의 맨 앞에 더미 노드 생성
    list->tail = NewNode(0);//연결 리스트의 맨 뒤에 더미 노드 생성
    list->head->next = list->tail;//head의 다음은 tail로 설정
    list->tail->prev = list->head;//tail의 다음은 head로 설정
    list->count = 0; 
}

void AddData(List *list, int pid){
    //새로 생성한 노드를 tail 앞으로
    Node *now = NewNode(pid);
    now->prev = list->tail->prev;
    now->next = list->tail;
    list->tail->prev->next = now;
    list->tail->prev = now;
    list->count++;
}

int popData(List * list){
	Node * temp;
	int result;
	result = list->head->next->data;
	temp = list->head->next;
	list->head->next = temp->next;
	free(temp);
	list->count--;
	return result;
}
 
void Remove(List *list, Node *now){
    now->prev->next = now->next;
    now->next->prev = now->prev;
    free(now);
    list->count--;
}
 
Node *Find(List *list, int pid){
    //head와 tail은 더미 노드입니다.
    Node *seek = list->head->next;
    while (seek != list->tail) 
    {
		printf("%d ", seek->data);
        if (pid == seek->data)//찾았을 때
        {
            return seek;
        }
        seek = seek->next;
    }
    return NULL;
}
 
void Clear(List *list){
    //head 다음 노드가 tail이 아닐 때까지 리스트에서 제거
    Node *seek = list->head->next;
    while (seek != list->tail)
    {
        seek = list->head->next;
    }
}