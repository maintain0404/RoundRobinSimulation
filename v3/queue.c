//큐 - 연결리스트 이용
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.c"

typedef struct QNode 
{
    prs_info * data;
    struct QNode *next;
}QNode;


typedef struct Queue 
{
    QNode *front; 
    QNode *rear;
    int count;
}Queue;

void InitQueue(Queue *queue);
int IsEmpty(Queue *queue); 
void Enqueue(Queue *queue, prs_info * data); 
prs_info * Dequeue(Queue *queue); 

void InitQueue(Queue *queue)
{
    queue->front = queue->rear = NULL; 
    queue->count = 0;
}

int IsEmpty(Queue *queue)
{
    return queue->count == 0;    
}

void Enqueue(Queue *queue, prs_info * data)
{
    QNode *now = (QNode *)malloc(sizeof(QNode)); 
    now->data = data;
    now->next = NULL;

    if (IsEmpty(queue))
    {
        queue->front = now;       
    }
    else
    {
        queue->rear->next = now;
    }
    queue->rear = now;
    queue->count++;
}

prs_info * Dequeue(Queue *queue)
{
    prs_info * re;
    QNode *now;
    if (IsEmpty(queue))
    {
        return NULL;
    }
    now = queue->front;
    re = now->data;
    queue->front = now->next;
    free(now);
    queue->count--;
    return re;
}
