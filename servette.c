#include <sys/types.h>
#include <sys/socket.h>
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
    return sock_stat;
}

void response(int client_sock)
{
    const char *echo_str = "HTTP/1.0 200 ok\n\n";
    const char *html_head = "<h1>Naive File Server</h1>\n";
    const char *tail = "</p></html>";
    char *ls_res = (char *)malloc(MAX_SIZE*sizeof(char));
    char *phead = "<p  style=\"white-space: pre-line;font-size: larger\">";
    run_command("ls -l",ls_res);
    //write(client_sock,echo_str,strlen(echo_str));
    write(client_sock,html_head,strlen(html_head));
    write(client_sock,phead,strlen(phead));
    write(client_sock,ls_res,strlen(ls_res));
    write(client_sock,tail,strlen(tail));
}
int main()
{   
    int server_sock,client_sock;
    server_sock = start_server();
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
        printf("accept\n");
        read(client_sock,buffer,MAX_SIZE);
        printf("%s\n\n",buffer);
        response(client_sock);
        close(client_sock);
        break;
    }
    close(server_sock);
    return 0;
}
