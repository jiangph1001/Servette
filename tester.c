//本程序用于测试Servette服务器的功能

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
//#include <bits/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>


#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#define TEST_PORT 18393
const char * file_base_path;

int start_client()
{
    int ret,sock_stat;
    struct sockaddr_in client_addr; //创建专用socket地址

    sock_stat = socket(PF_INET,SOCK_STREAM,0); //创建socket
    if(sock_stat < 0)
    {
        printf("socket error:%s\n",strerror(errno));
        exit(-1);
    }
    return sock_stat;

}
int main(int argc, char const *argv[])
{
    int client_sock,server_sock;
    client_sock = start_client();
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(connect(client_sock,(struct sockaddr *)& server_addr,sizeof(server_addr))< 0)
    {
        perror("connect error");
        return 2;
    }
    else
    {
        printf("connect!\n");
    }
    char buffer[256];
    char msg[256] = "GET / HTTP/1.1\r\nConnection: close\r\n\n";
    while(1)
    {
        send(client_sock,msg,strlen(msg),0);
        read(client_sock,buffer,24096);
        printf(">>>>%s<<<<\n",buffer);
        scanf("%s",msg);
    }
    return 0;
}
