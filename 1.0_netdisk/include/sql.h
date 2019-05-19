#ifndef __SQL_H__
#define __SQL_H__
#include "head.h"
#define STR_LEN 10

int sql_connect(MYSQL **conn);
int find_name(MYSQL *conn,char *name,char*);
void get_salt(char *str);
void add_user(MYSQL *conn,char *name,char *salt,char *mima);
int math_user(MYSQL *conn,char *name,char *password,char *token);
int math_token(MYSQL *conn,char *name,char *token);
#endif
