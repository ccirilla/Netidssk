#include "head.h"
#include <unistd.h>
struct sockaddr_in ser;

void epoll_func(int epfd,int fd,int caozuo,int duxie)
{
    struct epoll_event event;
    event.events = duxie;
    event.data.fd = fd;
    epoll_ctl(epfd,caozuo,fd,&event);
}
void ssend(int cFd,Train_t* ptrain)
{
    int i=3,ret;
    while(i--)
    {
        ret = send(cFd,ptrain,ptrain->Len+8,0);
        if(ret!=-1)
        {
            printf("sizeof(emun) = %ld\n",sizeof(ptrain->ctl_code));
            return;
        }
        else
        {
            //发token值；
            connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
        }
    }
    printf("服务器繁忙，请稍后再试\n");
    exit(0);
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

int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,3);
	int cFd,ret,type,epfd;
	bzero(&ser,sizeof(ser));
	ser.sin_family=AF_INET;
	ser.sin_port=htons(atoi(argv[2]));
	ser.sin_addr.s_addr=inet_addr(argv[1]);//点分十进制转为32位的网络字节序
    Zhuce login_msg;
    Train_t train;
    time_t now;
    struct epoll_event evs[10];
    char buf[100],*passwd,code[50]={"0"};
login_begin:
    system("clear");
    printf("请选择注册或者登陆：\n1:注册新账号\n2:已有账号直接登陆\n");
    scanf("%d",&type);
    if(type == 1)
    {
        printf("请输入要注册的用户名\n");
        scanf("%s",login_msg.name);
        passwd = getpass("请输入密码：");
        strcpy(buf,passwd);
        passwd = getpass("请再次输入密码确认：");
        if(strcmp(buf,passwd)!=0)
        {
            printf("两次输入不一致，即将回到主界面\n");
            sleep(1);
            goto login_begin;
        }else
        {
            strcpy(train.buf,login_msg.name);
            train.Len = strlen(train.buf)+1;
            train.ctl_code = LOGIN_PRE;
	        cFd=socket(AF_INET,SOCK_STREAM,0);
	        ERROR_CHECK(cFd,-1,"socket");
	        ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
	        ERROR_CHECK(ret,-1,"connect");
            printf("connect successful\n");
            ssend(cFd,&train);
        }
    }else
    {
        printf("请输入要登录的用户名\n");
        scanf("%s",login_msg.name);
        passwd = getpass("请输入密码：");
        strcpy(buf,passwd);
            strcpy(train.buf,login_msg.name);
            train.Len = strlen(train.buf)+1;
            train.ctl_code = REGISTER_PRE;
	        cFd=socket(AF_INET,SOCK_STREAM,0);
	        ERROR_CHECK(cFd,-1,"socket");
	        ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
	        ERROR_CHECK(ret,-1,"connect");
            printf("connect successful\n");
            ssend(cFd,&train);
    }
    epfd = epoll_create(1);
    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
    while(1)
    {
        int ready_num = epoll_wait(epfd,evs,10,-1);
        for(int i=0;i<ready_num;i++)
        {
            if(evs[i].events==EPOLLIN && evs[i].data.fd == cFd)
            {
                ret = one_recv(cFd,&train);
                if(ret == -1)
                {
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    cFd=socket(AF_INET,SOCK_STREAM,0);
                    ret=connect(cFd,(struct sockaddr*)&ser,sizeof(ser));
                    ERROR_CHECK(ret,-1,"connect");
                    printf("重连 successful\n");
                    epoll_func(epfd,cFd,EPOLL_CTL_ADD,EPOLLIN);
                    train.Len = sizeof(Zhuce);
                    memcpy(login_msg.passward,code,50);
                    memcpy(&train.buf,&login_msg,train.Len);
                    train.ctl_code = TOKEN_PLESE;
                    ret = send(cFd,&train,train.Len+8,0);
                    //重连，发token;重连要在服务器更新当前code值
                    //加入新的epoll
                    //此次SEND发name,tolen,当前code值；
                    continue;
                }
                switch(train.ctl_code)
                {
                case LOGIN_NO:
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    printf("账号已存在，请重新注册\n");
                    sleep(2);
                    goto login_begin;
                case LOGIN_POK:
                    strcpy(login_msg.token,train.buf);
                    strcpy(login_msg.passward,crypt(buf,login_msg.token));
                    printf("name = %s\nsalt = %s\npassward = %s\n",login_msg.name,
                           login_msg.token,login_msg.passward);
                    train.Len = sizeof(Zhuce);
                    memcpy(&train.buf,&login_msg,train.Len);
                    train.ctl_code = LOGIN_Q;
                    ret = send(cFd,&train,train.Len+8,0);
                    if(ret != -1)
                        printf("注册成功\n");
                    else
                        printf("网络繁忙，注册失败\n");
                    sleep(2);
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    goto login_begin;
                case REGISTER_NO:
                    printf("账号/密码错误，请重新登录\n");
                    close(cFd);
                    epoll_func(epfd,cFd,EPOLL_CTL_DEL,EPOLLIN);
                    sleep(2);
                    goto login_begin;
                case REGISTER_POK:
                    time(&now);
                    printf("salt = %s\n",train.buf);
                    strcpy(login_msg.passward,crypt(buf,train.buf));
                    sprintf(login_msg.token,"%s %s",login_msg.name,ctime(&now));
                    login_msg.token[strlen(login_msg.token)-1]=0;
                    printf("name = %s\ntoken = %s\npassward = %s\n",login_msg.name,
                           login_msg.token,login_msg.passward);
                    train.Len = sizeof(Zhuce);
                    memcpy(&train.buf,&login_msg,train.Len);
                    train.ctl_code = REGISTER_Q;
                    ret = send(cFd,&train,train.Len+8,0);
                    break;
                case REGISTER_OK:
                    printf("登录成功\n");
                    bzero(login_msg.passward,sizeof(login_msg.passward));
                    sleep(2);
                    system("clear");
                    break;
                }
            }
        }
    }
	return 0;
}
