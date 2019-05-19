#include "/home/zj/homework/ntedisk/include/factory.h"
#include "../include/ser_cli.h"
#include "../include/sql.h"
#include "../include/tranfile.h"

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
    File_info file_info;
    time_t now;
    time(&now);
    int log_fd = open("log.txt",O_RDWR|O_CREAT|O_APPEND,0666);
    dprintf(log_fd,"程序开始运行，当前时间\n%s\n",ctime(&now));
    gettimeofday(&start,NULL);
    MYSQL *conn;
    sql_connect(&conn);//连接数据库
    while(1)
    {
        int ready_num = epoll_wait(epfd,evs,SOCKET_NUM,5000);
        int nn_fd ;
        gettimeofday(&end,NULL);
        // 轮盘超时，关掉所有该轮盘值的new_fd；归零除name外三大状态；
        if((end.tv_sec-start.tv_sec)*1000000+end.tv_usec-start.tv_usec >5000000)
        {
            roud = (roud+1)%5;
            //printf("round = %d\n",roud);
            start = end;
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
                    log_name(log_fd,"超时断开",client[j].name);
                }
            }
        }
        for(int i=0;i<ready_num;i++)
        {
            nn_fd = evs[i].data.fd;
            //有人连接，更新client[fd]，lunpan更新,增加监控，client数目++
            if(evs[i].events == EPOLLIN && nn_fd == socket_fd)
            {
                time(&now);
                ret = sizeof(clien);
                new_fd = accept(socket_fd,(struct sockaddr*)&clien,&ret);
                log_port(log_fd,"请求连接","未知",&clien);
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
                continue;
            }
            //下载文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == down_thread[j].fd)
                {
                    read(nn_fd,&ret,4);
                }
            }
            //上传文件完成；
            for(int j=0;j<DOWN_THREAD_NUM;j++)
            {
                if(evs[i].events == EPOLLIN && nn_fd == down_thread[j].fd)
                {

                }
            }
            //已连接客户端提出请求
            if(client[nn_fd].state>0)
            {
                //有任何请求都更新圆盘值，若上传或者下载则从新置为-1；
                client[nn_fd].rotate = roud;
                ret = one_recv(nn_fd,&train);
                time(&now);
                if(ret == -1)
                {
                    log_name(log_fd,"客户退出",client[nn_fd].name);
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
                    dprintf(log_fd,"姓名: %-10s %-8s %s time: %s \n",train.buf,"注册请求",
                            client[nn_fd].name, ctime(&now)+8);
                    client[nn_fd].state =1;
                    strcpy(client[nn_fd].name,train.buf);
                    //判断这个id是否存在；
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
                    break;
                case LOGIN_Q://注册已经成功；
                    memcpy(&login_msg,train.buf,train.Len);
                    //printf("name = %s\nsalt = %s\npassward = %s\n",login_msg.name,
                    //       login_msg.token,login_msg.passward);
                    add_user(conn,login_msg.name,login_msg.token,login_msg.passward);
                    log_name(log_fd,"注册成功",client[nn_fd].name);
                    break;
                case REGISTER_PRE://登录请求
                    dprintf(log_fd,"姓名: %-10s %-8s %s time: %s \n",train.buf,"登录请求",
                            client[nn_fd].name, ctime(&now)+8);
                    client[nn_fd].state =1;
                    strcpy(client[nn_fd].name,train.buf);
                    ret = find_name(conn,client[nn_fd].name,train.buf);
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
                    ret = math_user(conn,login_msg.name,
                                    login_msg.passward,login_msg.token);
                    if(ret ==-1)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                        log_name(log_fd,"登录失败",client[nn_fd].name);
                    }else//登录成功，state=2，name,round更新;
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_OK;
                        strcpy(client[nn_fd].name,login_msg.name);
                        client[nn_fd].state =2;
                        log_name(log_fd,"登录成功",client[nn_fd].name);
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case TOKEN_PLESE:
                    memcpy(&login_msg,train.buf,train.Len);
                    log_name(log_fd,"重连请求",login_msg.name);
                    strcpy(client[nn_fd].name,login_msg.name);
                    client[nn_fd].code = atoi(login_msg.passward);
                    ret = math_token(conn,login_msg.name,login_msg.token);
                    if(ret == 0)
                    {
                        client[nn_fd].state =2;
                        log_name(log_fd,"重连成功",client[nn_fd].name);
                    }
                    else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                        send(nn_fd,&train,8+train.Len,0);
                        log_name(log_fd,"重连失败",client[nn_fd].name);
                    }
                    break;
                case LS_Q:
                    if(client[nn_fd].state ==2)
                    {
                        log_name(log_fd,"LS-L",client[nn_fd].name);
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
                        log_caozuo(log_fd,qq_msg.buf,qq_msg.buf1,client[nn_fd].name);
                        ret = operate_func(conn,&train,&qq_msg,client[nn_fd].name,&client[nn_fd].code);
                        //printf("ret = %d\n",ret);
                        if(ret == -1)
                        {
                            log_name(log_fd,"操作失败",client[nn_fd].name);
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                        }else
                            log_name(log_fd,"操作成功",client[nn_fd].name);
                    }else
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }
                    send(nn_fd,&train,8+train.Len,0);
                    break;
                case DOWNLOAD_PRE:
                    log_caozuo(log_fd,"下载文件",train.buf,client[nn_fd].name);
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else
                    {
                        strcpy(file_info.filename,train.buf);
                        ret = find_file_info(conn,&file_info,client[nn_fd].name,client[nn_fd].code);
                        if(ret ==-1)
                        {
                            train.Len = 0;
                            train.ctl_code = OPERATE_NO;
                        }else
                        {
                            memcpy(train.buf,&file_info,sizeof(File_info));
                            train.Len = sizeof(File_info);
                            train.ctl_code = DOWNLOAD_POK;
                        }
                        send(nn_fd,&train,8+train.Len,0);
                    }
                    break;
                case DOWNLOAD_Q:
                    if(client[nn_fd].state < 2)
                    {
                        train.Len = 0;
                        train.ctl_code = REGISTER_NO;
                    }else//准备移交权限;
                    {
                        ret =math_task(down_thread,DOWN_THREAD_NUM);
                        if(ret == -1)
                        {
                            train.Len = 0;
                            //应该再加个控制码表示服务器繁忙；
                            train.ctl_code = REGISTER_NO;
                            send(nn_fd,&train,8+train.Len,0);
                            break;
                        }
                        //任务转移；
                        printf("zuanyi\n");
                        write(down_thread[ret].fd,&nn_fd,4);
                        down_thread[ret].busy_num++;
                        epoll_func(epfd,nn_fd,EPOLL_CTL_DEL,EPOLLIN);
                        client[nn_fd].state = 0;
                        client[nn_fd].code = 0;
                        client[nn_fd].rotate = -1;
                        client_sum--;
                        log_name(log_fd,"下载转移",client[nn_fd].name);
                        break;
                    }

                }
            }
        }
    }
    return 0;
}
