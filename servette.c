#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>   
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "filemanage.h"
#include "config.h"

/*
Description:
    启动socket监听
Return:
    socket的文件描述符
*/
int start_server()
{
    int ret,sock_stat;
    struct sockaddr_in server_addr; //创建专用socket地址

    sock_stat = socket(PF_INET,SOCK_STREAM,0); //创建socket
    if(sock_stat < 0)
    {
        printf("socket error:%s\n",strerror(errno));
        exit(-1);
    }

    int opt = 1;
    setsockopt(sock_stat,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    server_addr.sin_family = AF_INET;  //ipv4
    server_addr.sin_port = htons(PORT); //port:8888
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    //将socket和socket地址绑定在一起
    ret = bind(sock_stat,(struct sockaddr *)&server_addr,sizeof(server_addr)); 
    if(ret != 0)
    {
        close(sock_stat);
        printf("bind error:%s\n",strerror(errno));
        exit(-2);
    }
    //开始监听
    ret = listen(sock_stat,5);
    if(ret != 0)
    {
        close(sock_stat);
        printf("listen error:%s\n",strerror(errno));
        exit(-3);
    }
    printf("Servette Start\nListening port:%d\n",PORT);
    return sock_stat;
}

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

/*
Description:
    读取html文件
Return:
    NULL
*/
void response_f(int client_sock, char *file)
{
    int fd;
    char buf[MAX_SIZE];
    ssize_t size = -1;
    const char *echo_str = "HTTP/1.1 200 ok\n\n";
    if(strcmp(file,"/")==0)
    {
        /*
        此处可能遗留bug,读取的文件强制指定为了
        程序目录下的文件,而不是根据配置文件中
        动态的修改，此问题可以解决但为了测试方
        便，当前版本暂不处理。
        */
        file = DEFAULT_FILE;
    }
    else  if(strcmp(file,"/favicon.ico")==0)
    {
        printf("reject /favicon.ico\n");
        write(client_sock,"\0",1);
        return;
    }
    else
    {
        char new_file[NAME_LEN];
        sprintf(new_file,"%s%s",DIR,file);
        printf("new file_path:%s\n",new_file);
        file = new_file;
    }
    
    fd = open(file,O_RDONLY);
    if(fd == -1)
    {
        fd = open("www/err.html",O_RDONLY);
    }
    write(client_sock,echo_str,strlen(echo_str));
    while(size)
    {
        //size代表读取的字节数
        size = read(fd,buf,MAX_SIZE);
        if(size > 0)
        {
            buf[size]='\0';
            //printf("%s",buf);
            write(client_sock,buf,size);
        }
    }
}


/*
Description:
    下载文件（运行不正常）
Return:
    NULL
*/
void response_d(int client_sock,char *arg)
{
    int fd;
    ssize_t size = -1;
    char buf[MAX_SIZE];
    char file_name[NAME_LEN];
    if(sscanf(arg,"/?file=%s",file_name)==EOF)
    {
        printf("error:%s\n",arg);
        return;
    }
    printf("\tdownloading %s\n",file_name);
    fd = open(file_name,O_RDONLY);
    if(fd == -1)
    {
        fd = open("www/err.html",O_RDONLY);
    }
    char *ct = "HTTP/1.1 200 ok\nContent-Type: txt\n\n";
    write(client_sock,ct,sizeof(ct));
    while(size)
    {
        //size代表读取的字节数
        size = read(fd,buf,MAX_SIZE);
        if(size > 0)
        {
            buf[size]='\0';
            //printf("%s",buf);
            send(client_sock,buf,size,0);
        }
    }
}

int main(int argc, char *argv[])
{   
    int server_sock,client_sock;
    server_sock = start_server();
    char methods[4];
    char file_path[64];
    while(1)
    {
        char buffer[MAX_SIZE];
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)& client_addr,&len);
        if(client_sock < 0)
        {
            continue;
        }
        read(client_sock,buffer,MAX_SIZE);
        //buffer是接收到的请求，需要处理
        //printf("%s\n\n",buffer);

        sscanf(buffer,"%s %s",methods,file_path);
        printf("Testing :%s %s\n",methods,file_path);
        switch(file_path[1]) 
        {
            case '?':
                response_d(client_sock,file_path);
                break;
            default:
                response_f(client_sock,file_path);
        }
        
        close(client_sock);
    }
    close(server_sock);
    return 0;
}
