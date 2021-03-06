#ifndef __FACTORY_H__
#define __FACTORY_H__
#include "head.h"
#include "work_que.h"
#define DOWN_THREAD_NUM 4
#define UP_THREAD_NUM 3
#define DOWN_TASK_NUM 1000
#define UP_TASK_NUM 10
#define MAX_CONNECT_NUM 3000
#define SOCKET_NUM 5000
#define AVG_CLIENT_NUM 30
typedef struct{
    pthread_t pid;
    int fd;
    int busy_num;//正在服务的客户数量
}process_data;
typedef struct{
    int state;
    char name[30];
    int code;
    int rotate;
}client_t;

void factoryInit(process_data*,process_data*,client_t*);
void factoryStart(process_data*,process_data*);
//void factoryDistory(client_t*);
int tcpInit(int*,FILE *);
void epoll_func(int,int,int,int);
int tranFile(int);
#endif
