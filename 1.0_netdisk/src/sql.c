#include "../include/sql.h"


int sql_connect(MYSQL **conn)
{
    char server[]="localhost";
    char user[]="root";
    char password[]="950711";
    char database[]="Netdisk";//要访问的数据库名称
    *conn=mysql_init(NULL);
    if(!mysql_real_connect(*conn,server,user,password,database,0,NULL,0))
    {
        printf("Error connecting to database:%s\n",mysql_error(*conn));
        exit(0);
    }else{
        printf("Connected...\n");
    }
    return 0;
}
int find_name(MYSQL *conn,char *name,char * salt)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select salt from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    puts(query);
    int t,ret =-1;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else{
        printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else
                {
                    if(salt !=NULL)
                        strcpy(salt,row[0]);
                    ret = 0;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
        return ret;
    }
}
void get_salt(char *str)
{
    str[STR_LEN + 1] = 0;
    int i,flag;
    srand(time(NULL));
    for(i = 0; i < STR_LEN; i ++)
    {
        flag = rand()%3;
        switch(flag)
        {
        case 0:
            str[i] = rand()%26 + 'a';
            break;
        case 1:
            str[i] = rand()%26 + 'A';
            break;
        case 2:
            str[i] = rand()%10 + '0';
            break;
        }
    }
    *str='$';str[1]='5';str[2]='$';
    return;
}
void add_user(MYSQL *conn,char *name,char *salt,char *mima)
{
    char query[200]="insert into User (name,salt,ciphertext)values(";
    sprintf(query,"%s'%s','%s','%s')",query,name,salt,mima);
    printf("query= %s\n",query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else{
        printf("insert success\n");
    }
}
int math_user(MYSQL *conn,char *name,char *password,char *token)
{
    int ret =-1;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select ciphertext from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else if(strcmp(row[0],password)!=0)
                {
                    ret = -1;
                }
                else
                {
                    ret = 0;
                    mysql_free_result(res);
                    char pquery[200]="update User set token='";
                    strcpy(query,"where name ='");
                    sprintf(pquery,"%s%s' %s%s'",pquery,token,query,name);
                    puts(pquery);
                    t=mysql_query(conn,pquery);
                    if(t)
                    {
                        printf("Error making query:%s\n",mysql_error(conn));
                    }else{
                        printf("update success\n");
                    }
                    return ret;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}
int math_token(MYSQL *conn,char *name,char *token)

{
    int ret =-1;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char query[300]="select token from User where name='";
    sprintf(query,"%s%s%s",query, name,"'");
    puts(query);
    int t;
    t=mysql_query(conn,query);
    if(t)
    {
        printf("Error making query:%s\n",mysql_error(conn));
    }else
    {
        printf("Query made...\n");
        res=mysql_use_result(conn);
        if(res)
        {
            if((row=mysql_fetch_row(res))!=NULL)
            {
                if(*row[0] == 0)
                    ret = -1;
                else if(strcmp(row[0],token)==0)
                {
                    ret = 0;
                }
            }
        }else{
            printf("查询出错\n");
            exit(0);
        }
        mysql_free_result(res);
    }
    return ret;
}


