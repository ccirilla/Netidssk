#ifndef __FUNC_H__
#define __FUNC_H__
#include <crypt.h>
#include<openssl/md5.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/msg.h>
#include <strings.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#define ARGS_CHECK(argc,val) {if(argc!=val) \
	{printf("error args\n");return -1;}}
#define ERROR_CHECK(ret,retval,funcName) {if(ret==retval) \
	{printf("%d:",__LINE__);fflush(stdout);perror(funcName);return -1;}}
#define THREAD_ERROR_CHECK(ret,funcName) {if(ret!=0) \
	{printf("%s:%s\n",funcName,strerror(ret));return -1;}}
typedef enum MSG_code{
    LOGIN_PRE=1,LOGIN_NO,LOGIN_POK,LOGIN_Q,LOGIN_OK,
    REGISTER_PRE,REGISTER_NO,REGISTER_POK,REGISTER_Q,REGISTER_OK,TOKEN_PLESE
}MSG_code;
typedef struct{
    char name[30];
    char passward[100];
    char token[50];
}Zhuce;
typedef struct{
	int Len;
    int  ctl_code;
	char buf[4096];
}Train_t;
#endif
