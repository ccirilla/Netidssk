#include "/home/zj/homework/ntedisk/include/factory.h"
#include "../include/ser_cli.h"
#include "../include/sql.h"

int main(int argc,char *argv[])
{
    ARGS_CHECK(argc,2);
    FILE *fp = fopen(argv[1],"r+");
    ERROR_CHECK(fp,NULL,"fopen");
    int socket_fd,new_fd;
    tcpInit(&socket_fd,fp);
    fclose(fp);
    client_t client[SOCKET_NUM];
    process_data down_thread[DOWN_THREAD_NUM];
    process_data upload_thread[UP_THREAD_NUM];
    factoryInit(down_thread,upload_thread,client);
    factoryStart(down_thread,upload_thread);
    int epfd = epoll_create(1);
    struct epoll_event evs[SOCKET_NUM];
    epoll_func(epfd,socket_fd,EPOLL_CTL_ADD,EPOLLIN);
    for(int i=0;i<DOWN_THREAD_NUM;i++)
        epoll_func(epfd,down_thread[i].fd,EPOLL_CTL_ADD,EPOLLIN);
    for(int i=0;i<UP_THREAD_NUM;i++)
        epoll_func(epfd,upload_thread[i].fd,EPOLL_CTL_ADD,EPOLLIN);
    int roud=0,client_sum=0,ret;
    struct sockaddr_in clien;
    struct timeval start,end;
    Train_t train;
    Zhuce login_msg;
    QUR_msg qq_msg;
    time_t now;
    time(&now);
    fp = fopen("log.txt","r+");
    fprintf(fp,"%s\n",ctime(&now));
    gettimeofday(&start,NULL);
    MYSQL *conn;
    int k=1;
    sql_connect(&conn);
    while(1)
    {
        int ready_num = epoll_wait(epfd,evs,SOCKET_NUM,5000);
        printf("第%d次收到包\n",k++);
        int nn_fd ;
        fflush(fp);
        gettimeofday(&end,NULL);
        if((end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec >5000000)
        {
            roud = (roud+1)%5;
            printf("8秒已到\n");
            for(int j=0;j<SOCKET_NUM;j++)
            {
                if(client[j].rotate == roud)
                {
                    epoll_func(epfd,j,EPOLL_CTL_DEL,EPOLLIN);
                    close(j);
                    client[j].state = 0;
                    client[j].code = 0;
                    client[j].rotate = -1;
                    client_sum--;
                    printf("已经断开这个B name: %s的连接\n",client[j].name);
                }
            }
        }
        for(int i=0;i<ready_num;i++)
        {
            nn_fd = evs[i].data.fd;
            printf("connect qian\n");
            //有人连接，更新client[fd]，lunpan更新,增加监控，client数目++
            if(evs[i].events == EPOLLIN && nn_fd == socket_fd)
            {
                time(&now);
                ret = sizeof(clien);
                new_fd = accept(socket_fd,(struct sockaddr*)&clien,&ret);
                fprintf(fp,"connect  ip:%s port:%d  time %s\n",inet_ntoa(clien.sin_addr),
                        ntohs(clien.sin_port),ctime(&now));
                if(client_sum >= MAX_CONNECT_NUM)//有时间再写一个缓冲队列
                    close(new_fd);
                else
                {
                    client[new_fd].state = 1;//1为已连接未登录成功状态；
                    client_sum++;
                    client[new_fd].code = 0;
                    client[new_fd].rotate = roud;
                    epoll_func(epfd,new_fd,EPOLL_CTL_ADD,EPOLLIN);
                    sprintf(client[new_fd].name,"ip:%s port:%d",
                            inet_ntoa(clien.sin_addr),ntohs(clien.sin_port));
                }
                printf("第%d个包用于connnect\n",k-1);
                continue;
            }
            //下载文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == down_thread[j].fd)
                {

                }
            }
            //上传文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == down_thread[j].fd)
                {

                }
            }
            printf("kehuqingqiuqian\n");
            //已连接客户端提出请求
            if(client[nn_fd].state>0)
            {
                //有任何请求都更新圆盘值，若上传或者下载则从新置为-1；
                client[nn_fd].rotate = roud;
                printf("round = %d\n",roud);
                ret = one_recv(nn_fd,&train);
                printf("longin: %d\n",train.ctl_code);
                time(&now);
                if(ret == -1)
                {
                    close(nn_fd);
                    client[nn_fd].state = 0;
                    client[nn_fd].code = 0;
                    client[nn_fd].rotate = -1;
                    client_sum--;
                    epoll_func(epfd,nn_fd,EPOLL_CTL_DEL,EPOLLIN);
                    continue;
                }
                switch(train.ctl_code)
                {
                case LOGIN_PRE:
                    fprintf(fp,"LOGIN_PRE  name: %s  %s  time: %s \n",train.buf,
                            client[nn_fd].name, ctime(&now));
                    client[nn_fd].state =1;
                    //client[nn_fd].rotate = roud;
                    strcpy(client[nn_fd].name,train.buf);
                    ret = find_name(conn,client[nn_fd].name,NULL);
                    if(ret ==0)
                    {
                        train.Len = 0;
                        train.ctl_code = LOGIN_NO;
                        send(nn_fd,&train,8,0);
                    }
                    else
                    {
                        get_salt(train.buf);
                        train.Len = strlen(train.buf)+1;
                        train.ctl_code = LOGIN_POK;
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    printf("第%d个包用于登录请求\n",k-1);
                    break;
                case LOGIN_Q://注册已经成功；
                    printf("shoudaoqingqiu\n");
                    memcpy(&login_msg,train.buf,train.Len);
                    printf("name = %s\nsalt = %s\npassward = %s\n",login_msg.name,
                           login_msg.token,login_msg.passward);
                    add_user(conn,login_msg.name,login_msg.token,login_msg.passward);
                    break;
                case REGISTER_PRE://登录请求
                    printf("name = %s\n",train.buf);
                    fprintf(fp,"REGISTER_PRE  name: %s  %s  time: %s \n",train.buf,
                            client[nn_fd].name, ctime(&now));
                    client[nn_fd].state =1;
                    //client[nn_fd].rotate = roud;
                    strcpy(client[nn_fd].name,train.buf);
                    ret = find_name(conn,client[nn_fd].name,train.buf);
                    printf("ret = %d\nsalt = %s\n",ret,train.buf);
                    if(ret ==-1)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        train.Len = strlen(train.buf)+1;
                        train.ctl_code = REGISTER_POK;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case REGISTER_Q:
                    memcpy(&login_msg,train.buf,train.Len);
                    printf("name = %s\ntoken = %s\npassward = %s\n",login_msg.name,
                           login_msg.token,login_msg.passward);
                    ret = math_user(conn,login_msg.name,
                                    login_msg.passward,login_msg.token);
                    printf("ret = %d\n",ret);
                    if(ret ==-1)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else//登录成功，state=2，name,round更新;
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_OK;
                        time(&now);
                        strcpy(client[nn_fd].name,login_msg.name);
                        printf("client[nn_fd].name = %s\n",client[nn_fd].name);
                        fprintf(fp,"REGISTER_OK  name: %s  time: %s \n",
                                client[nn_fd].name, ctime(&now));
                        client[nn_fd].state =2;
                        //client[nn_fd].rotate = roud;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case TOKEN_PLESE:
                    memcpy(&login_msg,train.buf,train.Len);
                    time(&now);
                    fprintf(fp,"TOKEN_PLESE  name: %s  %s  time: %s \n",login_msg.name,
                            client[nn_fd].name, ctime(&now));
                    strcpy(client[nn_fd].name,login_msg.name);
                    client[nn_fd].code = atoi(login_msg.passward);
                    ret = math_token(conn,login_msg.name,login_msg.token);
                    if(ret == 0)
                    {
                        client[nn_fd].state =2;
                        printf("name:%s 认证成功\n",client[nn_fd].name);
                        printf("code = :%d \n",client[nn_fd].code);
                    }
                    else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case LS_Q:
                    if(client[nn_fd].state ==2)
                    {
                        ls_func(conn,client[nn_fd].name,client[nn_fd].code,train.buf);
                        train.ctl_code = LS_OK;
                        train.Len = strlen(train.buf)+1;
                    }else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case OPERATE_Q:
                    if(client[nn_fd].state ==2)
                    {
                        memcpy(&qq_msg,train.buf,train.Len);
                        ret = operate_func(conn,&train,&qq_msg,client[nn_fd].name,&client[nn_fd].code);
                        printf("ret = %d\n",ret);
                        if(ret == -1)
                        {
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                        }
                    }else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                }
            }
        }
        printf("第%d次包结束\n",k-1);
    }
    return 0;
}
