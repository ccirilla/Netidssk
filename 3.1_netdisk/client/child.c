#include "child.h"

void add_task(DQ_buf *pdqbuf,CD_info *ptask,File_info *pfile,int cfd)
{
    Train_t train;
    char path[100]={0};
    struct stat stat;
    strcpy(path,DOWN_PATH);
    sprintf(path,"%s%s",path,pfile->filename);
    ptask->fd = open(path,O_RDWR|O_CREAT,0666);
    //如果文件大小已经等于最大，比对MD5，不等重新下；
    fstat(ptask->fd,&stat);
    ptask->pos = stat.st_size;
    memcpy(&ptask->file,pfile,sizeof(File_info));
    if(ptask->file.filesize > 100*1<<20)//mmap
    {
        ptask->state = 2;
        ftruncate(ptask->fd,ptask->file.filesize);
        ptask->p = (char*)mmap(NULL,ptask->file.filesize,PROT_READ|PROT_WRITE,
                               MAP_SHARED,ptask->fd,0);
    }else
    {
        ptask->state = 1;
        lseek(ptask->fd,ptask->pos,SEEK_SET);
    }
    pdqbuf->pos = ptask->pos;
    pdqbuf->fd = ptask->fd;
    memcpy(&pdqbuf->file,&ptask->file,sizeof(File_info));
    train.Len = sizeof(DQ_buf);
    train.ctl_code = DOWNLOAD_Q;
    memcpy(train.buf,pdqbuf,sizeof(DQ_buf));
    send(cfd,&train,train.Len+8,0);
}

void error_jude(int ret)
{
    if (ret ==-1)
    {
        printf("服务器断开,程序终止\n");
        exit(0);
    }
}

void *normal_func(void*pF)
{
    int fd = (int)pF,cfd=0;
    int ready_num;
    Train_t train;
    File_info file;
    CD_info task[10];
    DQ_buf dqbuf;
    int task_num =0,i,j,k,len,ret;
    bzero(task,10*sizeof(CD_info));
    int epfd = epoll_create(1);
    struct epoll_event event,evs[15];
    epoll_func(epfd,fd,EPOLL_CTL_ADD,EPOLLIN);

    while(1)
    {
        ready_num = epoll_wait(epfd,evs,15,-1);
        for( i=0;i<ready_num;i++)
        {
            //主线程分任务啦
            if(evs[i].events == EPOLLIN && evs[i].data.fd == fd)
            {
                read(fd,&file,sizeof(File_info));
                //上传任务;
                if(file.filesize<0)
                {

                }
                else//下载；
                {
                    if(task_num >=10)//任务为0时关闭描述符cfd,并置0
                    {
                        printf("下载任务已达到上限，请稍后再试\n");
                        continue;
                    }
                    if(task_num ==0)//第一次连接;
                    {
                        cfd = token_ident(epfd);//新建fd
         //               train.Len = 0;//用于服务器任务转移；
        //                train.ctl_code = DOWNLOAD_Q;
           //             send(cfd,&train,8+train.Len,0);
                    }
                    for(j =0;j<10;j++)
                        if(task[j].fd==0)//任务完成必须改为0；
                            break;
                    task_num++;
                    add_task(&dqbuf,task+j,&file,cfd);//已经发送请求;
                    printf("fd =%d,stat = %d,pos = %d,filename=%s\nfilesize = %d,md5sun= %s\n",task[j].fd,task[j].state,
                           task[j].pos,task[j].file.filename,task[j].file.filesize,
                           task[j].file.md5sum);
                    printf("tasknum =%d,已发送下载请求成功\n",task_num);
                }
            }
            if(evs[i].events == EPOLLIN && evs[i].data.fd == cfd)
            {
                ret = recvCYL(cfd,&len,4);
                error_jude(ret);
                ret = recvCYL(cfd,&k,4);
                error_jude(ret);
                for(j=0;j<10;j++)
                    if(task[j].fd ==k)
                        break;
                if(len !=0)
                {
                    if(task[j].state ==1)
                    {
                        ret = recvCYL(cfd,train.buf,len);
                        error_jude(ret);
                        task[j].pos += len;
                        write(task[j].fd,train.buf,len);
                        printf("-nomal file :%s have download %d 本次接收 %d",task[j].file.filename,
                               task[j].pos,len);
                        fflush(stdout);
                    }
                    if(task[j].state ==2)
                    {
                        ret = recvCYL(cfd,task[j].p+task[j].pos,len);
                        error_jude(ret);
                        task[j].pos += len;
                        printf("-mmap file :%s have download %d 本次接收 %d",task[j].file.filename,
                               task[j].pos,len);
                        fflush(stdout);
                    }
                }else
                {
                    printf("file %s下载完成\n",task[j].file.filename);
                    task_num--;
                    if(task_num ==0)
                    {
                        close(cfd);
                        epoll_func(epfd,cfd,EPOLL_CTL_DEL,EPOLLIN);
                        cfd = 0;
                    }
                    close(task[j].fd);
                    if(task[j].state ==2)
                        munmap(task[j].p,task[j].file.filesize);
                    bzero(task+j,sizeof(CD_info));
                }
            }

        }
    }
}

