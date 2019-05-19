#ifndef __WORK_QUE__
#define __WORK_QUE__
#include "head.h"

typedef struct tag_node
{
	int new_fd;
    char md5sum[50];
    off_t file_size;
    int client_fd;//客户端本地哪个FD
    int begin;
    int end;
}Node_t,*pNode_t;


typedef struct{
    pNode_t data;
    int tail,front;
	int queCapacity;//队列容量
	int queSize;//队列实时大小
}Que_t,*pQue_t;
void queInit(pQue_t,int);
void queInsert(pQue_t,int);
int queGet(pQue_t);
#endif
