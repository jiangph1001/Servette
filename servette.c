//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/socket.h>
//#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
//#include <unistd.h>
//#include <stdlib.h>   
//#include <string.h>
//#include <errno.h>
#include <pthread.h>

#include "response.h"

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


int main(int argc, char *argv[])
{   
    int server_sock,client_sock;
    server_sock = start_server();

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr *)& client_addr,&len);
        if(client_sock < 0)
        {
            continue;
        }
        //创建进程
        pthread_t tid;
        pthread_create(&tid,NULL,do_Method,&client_sock);
        pthread_join(tid,NULL);

    }
    close(server_sock);
    return 0;
}
