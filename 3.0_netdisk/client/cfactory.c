#include "cfactory.h"


void epoll_func(int epfd,int fd,int caozuo,int duxie)
{
    struct epoll_event event;
    event.events = duxie;
    event.data.fd = fd;
    epoll_ctl(epfd,caozuo,fd,&event);
}
void ssend(int cFd,int epfd,Train_t *ptrain)
{
    int ret = send(cFd,ptrain,ptrain->Len+8,0);
    if(ret ==-1)
    {
        Train_t new_train;
        memcpy(&new_train,ptrain,8+ptrain->Len);
        //printf("服务器繁忙，请稍后再试\n");
        close(cFd);
        epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
        cFd=socket(AF_INET,SOCK_STREAM,0);
        ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
        printf("重连 successful\n");
        epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
        ptrain->Len = sizeof(Zhuce);
       // memcpy(login_msg.passward,code,50);
        memcpy(ptrain->buf,&login_msg,ptrain->Len);
        ptrain->ctl_code = TOKEN_PLESE;
        ret = send(cFd,ptrain,ptrain->Len+8,0);
        ret = send(cFd,&new_train,new_train.Len+8,0);
    }
}
int recvCYL(int fd,void *pbuf,int len)
{
    char *buf = (char*)pbuf;
    int total = 0,ret;
    while(total<len)
    {
        ret = recv(fd,buf+total,len-total,0);
        if(ret == 0)
        {
            return -1;
        }
        total += ret;
        //    printf("%d  \n",total);
    }
    return 0;
}
int one_recv(int fd,Train_t *ptrain)
{
    int ret = recvCYL(fd,&ptrain->Len,4);
    ERROR_CHECK(ret,-1,"server let me go");
    ret = recvCYL(fd,&ptrain->ctl_code,4);
    ERROR_CHECK(ret,-1,"server let me go");
    ret = recvCYL(fd,&ptrain->buf,ptrain->Len);
    ERROR_CHECK(ret,-1,"server let me go");
    return 0;
}

int token_ident(int epfd)
{
    Train_t train;
    int cFd,ret;
    cFd=socket(AF_INET,SOCK_STREAM,0);
    ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
    ERROR_CHECK(ret,-1,"connect");
    printf("认证 successful\n");
    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
    train.Len = sizeof(Zhuce);
    memcpy(train.buf,&login_msg,train.Len);
    train.ctl_code = TOKEN_PLESE;
    ret = send(cFd,&train,train.Len+8,0);
    return cFd;
}
