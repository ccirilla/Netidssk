#ifndef __TRANFILE_H__
#define __TRANFILE_H__
#include "head.h"

#define DOWN_PATH "./Sdisk/"

typedef struct {
    char filename[30];
    int filesize;
    char md5sum[50];
}File_info;
typedef struct {
    int cfd;
    int state;
    char *p;
    int pos;
    File_info file;
    int sfd;
}CD_info;
typedef struct {
    int fd;
    int pos;
//    char client_name[30];
//    int code ;
    File_info file;
}DQ_buf;
typedef struct {
    int download_fd;
    int task_num;
    CD_info cdinfo[10];//发生变化必改状态位；
//    int s_fd[10];
}SD_info;
#endif
