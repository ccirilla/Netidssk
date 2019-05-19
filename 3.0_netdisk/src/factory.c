#include "../include/factory.h"
#include "../include/tranfile.h"
#include "../include/ser_cli.h"

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

int math_task(process_data *p,int max)
{
    int min =0;
    for(int i=1;i<max;i++)
    {
        if(p[i].busy_num<p[min].busy_num)
            min =i;
    }
    if(p[min].busy_num >=AVG_CLIENT_NUM)
        return -1;
    return min;
}

void disconnect(SD_info *p,int fd,int epfd)
{
    int i=1;
    close(p->download_fd);
    epoll_func(epfd,p->download_fd,EPOLL_CTL_DEL,EPOLLIN|EPOLLOUT);
    epoll_func(epfd,p->download_fd,EPOLL_CTL_DEL,EPOLLIN);
    for(int i=0;i<10;i++)
    {
        if(p->cdinfo[i].state==1)
            close(p->cdinfo[i].sfd);
        if(p->cdinfo[i].state==2)
        {
            munmap(p->cdinfo[i].p,p->cdinfo[i].file.filesize);
            close(p->cdinfo[i].sfd);
        }
    }
    bzero(p,sizeof(SD_info));
    write(fd,&i,4);
    printf("对方退出，所有下载已经取消\n");
}


void add_task(CD_info *ptask,DQ_buf *pdqbuf)
{
    char path[100]={0};
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pdqbuf->file.md5sum);
    ptask->sfd = open(path,O_RDONLY);
    if(ptask->sfd == -1)
    {
        printf("打开文件出错,程序终止\n");
        exit(0);
    }
    ptask->cfd =pdqbuf->fd;
    ptask->pos = pdqbuf->pos;
    memcpy(&ptask->file,&pdqbuf->file,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ptask->state = 2;
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ,
                               MAP_SHARED,ptask->sfd,0);
    }
    else
    {
        ptask->state = 1;
        lseek(ptask->sfd,ptask->pos,SEEK_SET);
    }
}

void del_task(CD_info *ptask)
{
    printf("file :%s 下载完成\n",ptask->file.filename);
    close(ptask->sfd);
    if(ptask->state ==2)
        munmap(ptask->p,ptask->file.filesize);
    bzero(ptask,sizeof(CD_info));
}

//void factoryDistory(process_data* down_thread,process_data* up_thread,client_t* client)
void* down_func(void *pF)
{
    SD_info client[AVG_CLIENT_NUM];
    bzero(client,AVG_CLIENT_NUM*sizeof(SD_info));
    int fd = (int)pF;
    int ready_num,ret,new_fd,i,j,k;
    DQ_buf dqbuf;
    Train_t train;
    printf("%ld  download thread get fd %d\n",pthread_self(),fd);
    int epfd = epoll_create(1);
    struct epoll_event event,evs[32];
    event.events = EPOLLIN;
    event.data.fd = fd;
    epoll_ctl(epfd,EPOLL_CTL_ADD,fd,&event);
    while(1)
    {
        ready_num = epoll_wait(epfd,evs,32,-1);
        for( i=0;i<ready_num;i++)
        {
            if(evs[i].events == EPOLLIN && evs[i].data.fd == fd)
            {
                read(fd,&new_fd,4);//一个客户发生控制转移只有一次
                for( j=0;j<AVG_CLIENT_NUM;j++)
                {
                    //一定能找到
                    if(client[j].download_fd == 0)//找到一个空闲的；
                        //     k=j;
                        // if(client[j].download_fd == new_fd)
                        break;
                }
                //  if(j == AVG_CLIENT_NUM)//没找到；
                epoll_func(epfd,new_fd,EPOLL_CTL_ADD,EPOLLIN);
                client[j].download_fd = new_fd;
                printf("I get task %d\n  and j= %d\n",new_fd,j);
                continue;
            }
            for(j=0;j<AVG_CLIENT_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && evs[i].data.fd ==  client[j].download_fd)
                {
                    printf("进来了收任务请求\n");
                    ret = recvCYL(client[j].download_fd,&dqbuf,sizeof(DQ_buf));
                    if(ret == -1)
                    {
                        disconnect(client+j,fd,epfd);
                        break;
                    }
                                    printf("name %s\nsize %d\n md5 %s\n",dqbuf.file.filename,
                                           dqbuf.file.filesize,dqbuf.file.md5sum);
                    if(client[j].task_num >=10)
                        disconnect(client+j,fd,epfd);
                    for(k =0;k<10;k++)
                        if(client[j].cdinfo[k].sfd ==0)
                            break;
                    client[j].task_num++;
                    add_task(client[j].cdinfo+k,&dqbuf);
                    epoll_func(epfd,client[j].download_fd,EPOLL_CTL_MOD,EPOLLOUT|EPOLLIN);
                    printf("cfd = %d,stat =%d,pso = %d,sfd = %d,file = %s\n",client[j].cdinfo[k].cfd,client[j].cdinfo[k].state,
                           client[j].cdinfo[k].pos,client[j].cdinfo[k].sfd,client[j].cdinfo[k].file.filename);
                }
                if(evs[i].events == EPOLLOUT && evs[i].data.fd ==  client[j].download_fd)
                {
                    usleep(800000);
         //           printf("client[j].download_fd = %d\n",client[j].download_fd);
                    CD_info *pc=client[j].cdinfo;
                    int slice = (BUFSIZE)/client[j].task_num;
                    for(k =0;k<10;k++)
                    {
                        if(pc[k].state ==  1)//正常小火车
                        {
          //          printf("client[j].cfd = %d\n",client[j].cdinfo[k].cfd);
                            if(pc[k].pos < pc[k].file.filesize)
                            {
                                ret = read(pc[k].sfd,train.buf,slice);
                                train.ctl_code = pc[k].cfd;
                                train.Len =ret;
                                pc[k].pos += ret;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                    disconnect(client+j,fd,epfd);
                                else
                                    printf("-nomal file :%s have download %d 本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret-8);
                            }else
                            {
                                train.ctl_code = pc[k].cfd;
                                train.Len =0;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                    disconnect(client+j,fd,epfd);
                                client[j].task_num--;
                         //       printf("已经发送len=0\n");
                                if(client[j].task_num ==0)
                                    disconnect(client+j,fd,epfd);
                                else
                                    del_task(&client[j].cdinfo[k]);
                            }
                        }
                        if(pc[k].state == 2)
                        {
                            if(pc[k].pos + slice < pc[k].file.filesize)
                            {
                                memcpy(train.buf,pc[k].p+pc[k].pos,slice);
                                train.ctl_code = pc[k].cfd;
                                train.Len =slice;
                                pc[k].pos += slice;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                    disconnect(client+j,fd,epfd);
                                else
                                    printf("-mmap file :%s have download %d 本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret-8);
                            }else
                            {
                                memcpy(train.buf,pc[k].p+pc[k].pos,pc[k].file.filesize-pc[k].pos);
                                train.ctl_code = pc[k].cfd;
                                train.Len =pc[k].file.filesize-pc[k].pos;
                                pc[k].pos =pc[k].file.filesize;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                    disconnect(client+j,fd,epfd);
                                else
                                    printf("-trail mmap file :%s have download %d本次发送 %d\n",pc[k].file.filename,pc[k].pos,ret);
                                train.ctl_code = pc[k].cfd;
                                train.Len =0;
                                ret = send(client[j].download_fd,&train,8+train.Len,0);
                                if(ret == -1)
                                    disconnect(client+j,fd,epfd);
                                client[j].task_num--;
                                if(client[j].task_num ==0)
                                    disconnect(client+j,fd,epfd);
                                else
                                    del_task(&client[j].cdinfo[k]);
                            }

                        }
                    }
                }

            }
        }
    }
}
