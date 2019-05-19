#ifndef __CHILD_H__
#define __CHILD_H__
#include "head.h"
#include "cfactory.h"
#define DOWN_PATH "./Cdisk/"

extern struct sockaddr_in ser;
extern Zhuce login_msg;
extern char path[];

typedef struct{
    pthread_t pid;
    int fd;
    int busy_num;
}process_data;

void *normal_func(void*);


#endif
