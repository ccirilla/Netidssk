#include "../include/factory.h"

void log_name(int fd,const char *caozuo,char *name)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-8s  time: %s\n",name,caozuo,p+8);
}
void log_caozuo(int fd,const char *caozuo,char *neirong,char *name)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-4s %-6s time: %s\n",name,caozuo,neirong,p+8);
}
void log_port(int fd,const char *caozuo,char *name,struct sockaddr_in*clien)
{
    time_t now;
    time(&now);
    char *p =ctime(&now);
    p[strlen(p)-5]=0;
    dprintf(fd,"账号: %-10s %-8s  time: %s ip:%s 端口:%d\n",name,caozuo,p+8,
            inet_ntoa(clien->sin_addr),ntohs(clien->sin_port));
}

void* down_func(void *pF)
{
    int fd = (int)pF;
    int ready_num;
    printf("%ld  download thread get fd %d\n",pthread_self(),fd);
    int epfd = epoll_create(1);
    struct epoll_event event,evs[32];
    event.events = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    while(1)
    {
        ready_num = epoll_wait(epfd,evs,32,-1);
        printf("thread: %ld  get tesk\n",pthread_self());
        close(fd);
        printf("thread: %ld  finish tesk\n",pthread_self());
    }
}

void* upload_func(void *pF)
{
    int fd = (int)pF;
    int ready_num;
    printf("%ld  upload thread get fd %d\n",pthread_self(),fd);
    int epfd = epoll_create(1);
    struct epoll_event event,evs[32];
    event.events = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    while(1)
    {
        ready_num = epoll_wait(epfd,evs,32,-1);
        printf("thread: %ld  get tesk\n",pthread_self());
        close(fd);
        printf("thread: %ld  finish tesk\n",pthread_self());
    }
}
void factoryInit(process_data* down_thread,process_data* up_thread,client_t* client)
{
    bzero(down_thread,sizeof(process_data)*DOWN_THREAD_NUM);
    bzero(up_thread,sizeof(process_data)*DOWN_THREAD_NUM);
    bzero(client,sizeof(client_t)*SOCKET_NUM);
    for(int i=0;i<SOCKET_NUM;i++)
        client[i].rotate = -1;
}
void factoryStart(process_data* down_thread,process_data* up_thread)
{
    int i;
    int fds[2];
    printf("now will start downlod thread\n");
    for(i=0;i<DOWN_THREAD_NUM;i++)
    {
        socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
        pthread_create(&down_thread[i].pid,NULL,down_func,(void*)fds[1]);
        down_thread[i].fd = fds[0];
    }
    printf("now will start upload thread\n");
    for(i=0;i<UP_THREAD_NUM;i++)
    {
        socketpair(AF_LOCAL,SOCK_STREAM,0,fds);
        pthread_create(&up_thread[i].pid,NULL,upload_func,(void*)fds[1]);
        up_thread[i].fd = fds[0];
    }
    printf("all pthread are ready\n");
}
void epoll_func(int epfd,int fd,int caozuo,int duxie)
{
    struct epoll_event event;
    event.events = duxie;
    event.data.fd = fd;
    epoll_ctl(epfd,caozuo,fd,&event);
}
void factoryDistory(process_data* down_thread,process_data* up_thread,client_t* client)
{
}
