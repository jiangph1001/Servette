
#include <stdio.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/thread.h>

#include <pthread.h>
#include <pwd.h>
#include "response.h"

char * file_base_path;


/*
Description:
    读事件回调函数
Parameters:
    struct bufferevent *bev [IN]
    void *arg [IN] 自定义的参数,即_new_bind函数中的第三个参数
Return:
    NULL
*/
void socket_read_cb(struct bufferevent *bev, void *arg) 
{
    char buffer[MAX_SIZE];
    size_t len = bufferevent_read(bev, buffer, sizeof(buffer) - 1);

    buffer[len] = '\0';
    printf("%s\n", buffer);

    char reply[] = "HTTP/1.1 200 OK\r\n\r\n <h1>test</h1>";

    bufferevent_write(bev, reply, strlen(reply));
    /*
    char test[] = "1\n";
    while(1)
    {
        bufferevent_write(bev, test, strlen(test));
        usleep(5000);
    }
    */
}

/*
Description:
    写回调，占位
*/
void socket_write_cb(struct bufferevent *bev, void *arg)
{
    //do_nothing
}

/*
Description:
    事件回调函数
Parameters:
    struct bufferevent *bev [IN]
    short events [IN]
    void *arg [IN] 自定义的参数,即_new_bind函数中的第三个参数
Return:
    NULL
*/
void socket_event_cb(struct bufferevent *bev, short events, void *arg) {
    if (events & BEV_EVENT_EOF) {
        printf("connection closed\n");
    }
    else if (events & BEV_EVENT_ERROR) {
        printf("some other error\n");
    }

    bufferevent_free(bev);
}

/*
Description:
    listener接受连接时的回调函数 
Parameters:
    struct evconnlistener *listener [IN] 
    evutil_socket_t fd [IN] 文件描述符
    struct sockaddr *sock [IN] 服务端sockaddr
    int socklen [IN] 长度
    void *arg [IN] 自定义的参数,即_new_bind函数中的第三个参数
Return:
    NULL
*/
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sock, int socklen, void *arg) 
{

    //printf("accept a client %d\n", fd);
    struct event_base *base;
    struct bufferevent *bev;

    base = (struct event_base *) arg;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    //第三个参数为write_cb，暂不设置
    //
    bufferevent_setcb(bev, socket_read_cb, socket_write_cb, socket_event_cb, NULL);
    bufferevent_enable(bev, EV_READ | EV_PERSIST);

}
int main(int argc, const char* argv[])
{
    struct event_base* base;
    struct sockaddr_in server_addr;
    struct bufferevent* bev;
 

    base = event_base_new();
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

    struct evconnlistener *listener = evconnlistener_new_bind(base,listener_cb,base,
                                    LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,10,
                                    (struct sockaddr*)& server_addr,sizeof(struct sockaddr_in));
    
    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);
 
    return 0;

}