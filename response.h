// 此头文件用于处理响应
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "http_header_utils.h"
#include "filemanage.h"
/*
//临时测试用
void response_test(int client_sock)
{
    const char *echo_str = "HTTP/1.1 200 ok\r\n";
    const char *html_head = "<h1>Naive File Server</h1>\n";
    const char *tail = "</p></html>";
    char *ls_res = (char *)malloc(MAX_SIZE*sizeof(char));
    char *phead = "<p  style=\"white-space: pre-line;font-size: larger\">";
    run_command("ls -l",ls_res);
    write(client_sock,echo_str,strlen(echo_str));
    write(client_sock,html_head,strlen(html_head));
    write(client_sock,phead,strlen(phead));
    write(client_sock,ls_res,strlen(ls_res));
    write(client_sock,tail,strlen(tail));
}
*/

/*
Description:
    生成响应头
    从简，不返回更多的响应头
Parameters:
    char *header [OUT] 输出的响应头
    int status [IN] 状态码
    const char *type [IN] 内容类型
Return:
    NULL
*/
void construct_header(char *header,int status,const char *type)
{
    char msg[MIN_SIZE];
    switch(status)
    {
        case 200:
            sprintf(msg,"%s","OK");
            break;
        case 404:
            sprintf(msg,"%s","Not Found");
            break;
        case 204:
            sprintf(msg,"%s","No Content");
            break;
        default:
            sprintf(msg,"%s","Unset");
            break;
    }
    sprintf(header,"HTTP/1.1 %d %s\r\n",status,msg);
    sprintf(header,"%sContent-Type:%s\r\n\n",header,type);
    printf("%s\n",header);

}
/*
Description:
    生成响应头for Download！
Parameters:
    char *header [OUT] 输出的响应头
    int status [IN] 状态码
    const char *file_name [IN] 下载的文件名
Return:
    NULL
*/
void construct_download_header(char *header,int status,const char * file_name)
{
    sprintf(header,"HTTP/1.1 %d ok\r\n",status);//暂时忽略状态信息，默认ok
    //Content-Type:application/octet-stream为下载类型
    sprintf(header,"%sContent-Type:application/octet-stream\r\n",header);
    sprintf(header,"%sContent-Disposition: attachment;filename=%s\r\n\n",header,file_name);
    printf("%s\n",header);
}


/*
Description:
    读取html文件，并显示在网页上
Parameters:
    int client_sock [IN] 客户端的socket
    char *file: [IN] 解析的html文件名
Return:
    NULL
*/
void response_f(int client_sock, char *file)
{
    int fd;
    char buf[MAX_SIZE],header[MAX_SIZE];
    ssize_t size = -1;
    if(strcmp(file,"/")==0)
    {
        //如果没有指定文件，则默认打开index.html
        sprintf(file,"%s%s",DIR,DEFAULT_FILE);
    }
    else  if(strcmp(file,"/favicon.ico")==0)
    {
        //对浏览器请求图标的行为返回一个空包
        //如果不返回包，tcp包会多次超时重传
        printf("reject /favicon.ico\n");
        write(client_sock,"\0",1);
        return;
    }
    else
    {
        char new_file[NAME_LEN];
        sprintf(new_file,"%s%s",DIR,file);
        file = new_file;
    }
    //读取文件
    fd = open(file,O_RDONLY);
    if(fd == -1)
    {
        construct_header(header,404,"text/html");
        write(client_sock,header,strlen(header));
        return;
    }
    else
    {
        construct_header(header,200,"text/html");
        write(client_sock,header,strlen(header));
        //write(client_sock,echo_str,strlen(echo_str));
        while(size)
        {
            //size代表读取的字节数
            size = read(fd,buf,MAX_SIZE);
            if(size > 0)
            {
                write(client_sock,buf,size);
            }
        }
        char *ls_res = (char *)malloc(MAX_SIZE*sizeof(char));
        char *phead = "<p  style=\"white-space: pre-line;font-size: larger\">";
        const char *tail = "</p></html>";
        run_command("ls -l",ls_res);
        write(client_sock,phead,strlen(phead));
        write(client_sock,ls_res,strlen(ls_res));
        write(client_sock,tail,strlen(tail));
    }
}


/*
Description:
    处理下载文件的请求/?download=[文件名]
Parameters:
    int client_sock [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_download(int client_sock,char *arg)
{
    int fd;
    ssize_t size = -1;
    char buf[MAX_SIZE],header[MAX_SIZE];
    char file_name[NAME_LEN];
    if(sscanf(arg,"/?download=%s",file_name)==EOF)
    {
        //匹配失败！
        printf("error:%s\n",arg);
        construct_header(header,404,"text/html");
        write(client_sock,header,strlen(header));
        return;
    }
    fd = open(file_name,O_RDONLY);
    if(fd == -1)
    {
        construct_header(header,404,"text/html");
        write(client_sock,header,strlen(header));
        return;
    }
    //构建下载的相应头部
    printf("\tdownloading %s\n",file_name);
    construct_download_header(header,200,file_name); 
    write(client_sock,header,strlen(header));
    while(size)
    {
        //size代表读取的字节数
        size = read(fd,buf,MAX_SIZE);
        if(size > 0)
        {
            send(client_sock,buf,size,0);
        }
    }
    printf("\n");
}


/*
Description:
    从输入的buffer字符数组中的下标pos开始，到下一个"\r\n"。
    获取完整的一行。输入的数字代表限制长度。此函数仅仅可以
    在
Return:
    返回下一行第一个字符的位置
*/
ssize_t get_next_line(char * temp, const char * buffer, ssize_t pos_of_buffer, ssize_t limit)
{
    ssize_t pos_of_temp = 0;
    for( ; pos_of_temp < limit; )
    {
        if( buffer[pos_of_buffer] == '\r' || buffer[pos_of_buffer + 1] == '\n' )
            break;
        else
            temp[pos_of_temp++] = buffer[pos_of_buffer++]; 
    }
    temp[pos_of_temp] = '\0';

    // 这里还需要处理！！！
    return (pos_of_buffer + 2);
}


void upload_file(const char * buffer, http_header_chain headers, ssize_t begin_pos_of_http_content)
{
    int fd = open("test.txt", O_RDWR|O_CREAT);
    char temp[MAX_SIZE];
    //begin_pos_of_http_content 很重要
    //从首部行Content-Type中获得分隔符boundary
    get_http_header_content("Content-Type", temp, &headers, MAX_SIZE);
    ssize_t pos_of_buffer = begin_pos_of_http_content;
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    int n = write(fd, temp, strlen(temp) );
}

/*
Description:
    对请求的内容进行处理，分割为GET和POST请求
Parameters:
    int client_sock [IN] 客户端的socket
    char *buffer: [IN] 请求内容
Return:
    NULL
*/
void do_Method(int client_sock,char *buffer)
{
      
    char methods[4]; //GET or POST
    char file_path[NAME_LEN];
    sscanf(buffer,"%s %s",methods,file_path);
    printf("%s %s\n",methods,file_path);

    //获得所有的首部行组成的链表
    http_header_chain headers = (http_header_chain)malloc(sizeof(_http_header_chain));
    //begin_pos_of_http_content是buffer中可能存在的HTTP内容部分的起始位置, GET报文是没有的，POST报文有
    ssize_t begin_pos_of_http_content = get_http_headers(buffer,&headers);

    switch(methods[0])
    {
        // GET
        case 'G':
            switch(file_path[1]) 
            {
                case '?':
                    response_download(client_sock,file_path);
                    break;
                default:
                    response_f(client_sock,file_path);
            }
            break;
            // POST
        case 'P':
            upload_file(buffer, headers, begin_pos_of_http_content);
            break;
    }
    delete_http_headers(&headers);
}


