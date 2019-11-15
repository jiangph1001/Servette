#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define BUFFER 1024

/*
Description：
    获取当前所在的目录
Parameters:
    const char* cmd [IN] 要执行的指令
    char* res: [OUT] 接收输出的结果
Return:
    NULL
*/
void run_command(const char* cmd,char* res)
{
    int len=0;
    char *data;
    FILE *pp = popen(cmd, "r"); //建立管道
    if (!pp) {
        perror("popen error");
        exit(-1);
    }
    data = (char *)malloc(BUFFER*sizeof(char));
    while (fgets(data, sizeof(data), pp) != NULL)
	{
		sprintf(res,"%s%s",res,data);
	}
    puts(res);
	free(data);
    pclose(pp); //关闭管道
}


/*
Description：
    获取当前所在的目录
Parameters:
    int len: [IN] 路径的长度
    char *path: [OUT] 接收输出的结果
Return:
    NULL
*/
void get_pwd(char *path,int len)
{
    
    if(!getcwd(path,len))
    {
        perror("getcwd error");
        exit(1);
    }
}


/*
Description：
    列出指定路径下的文件
Parameters:
    const char *path: [IN] 指定的路径
    char *res: [OUT] 接收输出的结果
Return:
    null
*/
void ls(const char *path, char *res)
{
    DIR *dp;
    struct dirent *dirp;
    res = (char *)malloc(BUFFER*sizeof(char));
    dp=opendir(path);
    if(dp==NULL)
    {
        printf("error");
        exit(-1);
    }
    while((dirp=readdir(dp))!=NULL)
    {
        strcat(res,dirp->d_name);
    }
    puts(res);
    closedir(dp);
}
