#ifndef __SER_CLI_H__
#define __SER_CLI_H__
#include "head.h"
typedef enum MSG_code{
    LOGIN_PRE=1,LOGIN_NO,LOGIN_POK,LOGIN_Q,LOGIN_OK,
    REGISTER_PRE,REGISTER_NO,REGISTER_POK,REGISTER_Q,REGISTER_OK,TOKEN_PLESE
}MSG_code;
typedef struct{
    char name[30];
    char passward[100];
    char token[50];
}Zhuce;
int recvCYL(int fd,void *pbuf,int len);
int one_recv(int fd,Train_t *ptrain);
#endif

