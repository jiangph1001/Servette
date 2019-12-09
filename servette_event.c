
#include <stdio.h>
#include <event2/event.h>

#include <event.h>
#include <pthread.h>
#include <pwd.h>
#include "response.h"

struct sock_ev_write
{ //用户写事件完成后的销毁，在on_write()中执行
    struct event *write_ev;
    char *buffer;
};
struct sock_ev
{                            //用于读事件终止（socket断开）后的销毁
    struct event_base *base; //因为socket断掉后，读事件的loop要终止，所以要有base指针
    struct event *read_ev;
};

char *file_base_path;

/**
 * 销毁写事件用到的结构体
 */
void destroy_sock_ev_write(struct sock_ev_write* sock_ev_write_struct){
    if(NULL != sock_ev_write_struct){
//        event_del(sock_ev_write_struct->write_ev);//因为写事件没用EV_PERSIST，故不用event_del
        if(NULL != sock_ev_write_struct->write_ev){
            free(sock_ev_write_struct->write_ev);
        }
        /*
        if(NULL != sock_ev_write_struct->buffer){
            delete []sock_ev_write_struct->buffer;
        }
        */
        free(sock_ev_write_struct);
    }
}


/**
 * 读事件结束后，用于销毁相应的资源
 */
void destroy_sock_ev(struct sock_ev* sock_ev_struct){
    if(NULL == sock_ev_struct){
        return;
    }
    event_del(sock_ev_struct->read_ev);
    event_base_loopexit(sock_ev_struct->base, NULL);//停止loop循环
    if(NULL != sock_ev_struct->read_ev){
        free(sock_ev_struct->read_ev);
    }
    event_base_free(sock_ev_struct->base);
//    destroy_sock_ev_write(sock_ev_struct->sock_ev_write_struct);
    free(sock_ev_struct);
}

void on_write(int client_sock, short event, void *arg)
{
    printf("on write\n");
    char methods[5],message[MIDDLE_SIZE];; //GET or POST
    if (arg == NULL)
    {
        return;
    }
    struct sock_ev_write *sock_ev_write_struct = (struct sock_ev_write *)arg;

    char *buffer;
    buffer = (char *)malloc(MAX_SIZE*sizeof(char));
    sprintf(buffer, "%s",sock_ev_write_struct->buffer);

    //destroy_sock_ev_write(sock_ev_write_struct);
    //int write_num = write(sock, buffer, strlen(buffer));
    int s_ret = sscanf(buffer, "%s %s", methods, message);
    if (s_ret != 2)
    {
        //如果buffer中的数据无法被分割为两个字符串，则默认echo
        response_echo(client_sock, buffer);
        return;
    }
    //获得所有的首部行组成的链表
    http_header_chain headers = (http_header_chain)malloc(sizeof(_http_header_chain));
    //begin_pos_of_http_content是buffer中可能存在的HTTP内容部分的起始位置, GET报文是没有的，POST报文有
    int begin_pos_of_http_content = get_http_headers(buffer, &headers);
    //print_http_headers(&headers);
    switch (methods[0])
    {
        // GET
        case 'G':
            // 当message字符串开始就出现"/?download="子串时
            if (kmp(message, "/?download=", strlen(message)) == 0)
            {
                response_download_chunk(client_sock, message);
            }
            else if (kmp(message, "/?cgi-bin=", strlen(message)) == 0)
            // 当message字符串开始就出现"/?cgi-bin="子串时
            {
                response_cgi(client_sock, message);
            }
            else
            {
                response_webpage(client_sock, message);
            }
            break;
            // POST
        case 'P':
            upload_file(client_sock, buffer, message, headers, begin_pos_of_http_content, strlen(buffer));
            break;
        default:
            printf("不支持的请求:\n");
            response_echo(client_sock, buffer);
    }
    //usleep(40000);
}

void on_read(int client_sock, short event, void *arg)
{
    struct sock_ev *event_struct = (struct sock_ev *)arg; //获取传进来的参数
    char *buffer;
    buffer = (char *)malloc(MAX_SIZE * sizeof(char));
    /*
    int flags;
    flags = fcntl(client_sock, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(client_sock, F_SETFL, flags);
    */
    //本来应该用while一直循环，但由于用了libevent，只在可以读的时候才触发on_read()
    int size_of_buffer = recv(client_sock, buffer, MAX_SIZE, 0); 
    if (size_of_buffer == 0)
    { //说明socket关闭
        //destroy_sock_ev(event_struct);
        //close(client_sock);
        return;

    }
    printf("%s\n", buffer);
    struct sock_ev_write *sock_ev_write_struct = (struct sock_ev_write *)malloc(sizeof(struct sock_ev_write));
    sock_ev_write_struct->buffer = buffer;
    struct event *write_ev = (struct event *)malloc(sizeof(struct event)); //发生写事件（也就是只要socket缓冲区可写）时，就将反馈数据通过socket写回客户端
    sock_ev_write_struct->write_ev = write_ev;
    event_set(write_ev, client_sock, EV_WRITE, on_write, sock_ev_write_struct);
    event_base_set(event_struct->base, write_ev);
    event_add(write_ev, NULL);
}



void *new_process(void *p_client_sock)
{
    //线程分离
    pthread_detach(pthread_self());
    int client_sock = *(int *)p_client_sock;
    //初始化base,写事件和读事件
    struct event_base *base = event_base_new();
    struct event *read_ev = (struct event *)malloc(sizeof(struct event)); //发生读事件后，从socket中取出数据

    //将base，read_ev,write_ev封装到一个event_struct对象里，便于销毁
    struct sock_ev *event_struct = (struct sock_ev *)malloc(sizeof(struct sock_ev));
    event_struct->base = base;
    event_struct->read_ev = read_ev;
    //对读事件进行相应的设置
    event_set(read_ev, client_sock, EV_READ | EV_PERSIST, on_read, event_struct);
    event_base_set(base, read_ev);
    event_add(read_ev, NULL);
    //开始libevent的loop循环
    event_base_dispatch(base);
}
/*
Description:
    每当有新连接连到server时，就通过libevent调用此函数。
    每个连接对应一个新线程
 */
void on_accept(int sock, short event, void *arg)
{
    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        
        int client_sock = accept(sock, (struct sockaddr *)&client_addr, &len);
        if (client_sock < 0)
        {
            printf("accept error\n");
            return;
        }
        printf("new accept :%d\n", client_sock);
        //accept_new_thread(new_fd);
        pthread_t tid;
        pthread_create(&tid, NULL, new_process, &client_sock);
        //pthread_join(tid, NULL); // // // //
    }
}

/*
Description:
    启动socket监听
Return:
    socket的文件描述符
*/
int start_server()
{
    int ret, sock_stat;
    struct sockaddr_in server_addr; //创建专用socket地址

    sock_stat = socket(PF_INET, SOCK_STREAM, 0); //创建socket
    if (sock_stat < 0)
    {
        printf("socket error:%s\n", strerror(errno));
        exit(-1);
    }

    //使用event设置reuse,等同于setsockopt(SO_REUSEADDR)
    //evutil_make_listen_socket_reuseable(sock_stat);
    //设置接收地址和端口重用
    int opt = 1;
    setsockopt(sock_stat,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    setsockopt(sock_stat,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                //ipv4
    server_addr.sin_port = htons(PORT);              //port:8080
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0
    //将socket和socket地址绑定在一起
    ret = bind(sock_stat, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret != 0)
    {
        close(sock_stat);
        printf("bind error:%s\n", strerror(errno));
        exit(-2);
    }
    //开始监听
    ret = listen(sock_stat, 5);
    if (ret != 0)
    {
        close(sock_stat);
        printf("listen error:%s\n", strerror(errno));
        exit(-3);
    }

    //设置libevent事件，每当socket出现可读事件，就调用on_accept()
    printf("Servette Start\nPowered By libevent\nListening port:%d\n", PORT);
    struct event_base* base = event_base_new();
    struct event listen_ev;
    event_set(&listen_ev, sock_stat, EV_READ|EV_PERSIST, on_accept, NULL);
    event_base_set(base, &listen_ev);
    event_add(&listen_ev, NULL);
    event_base_dispatch(base);

    event_del(&listen_ev);
    event_base_free(base);
    return sock_stat;
}

int main(int argc, char *argv[])
{
    int server_sock, client_sock;
    struct passwd *pwd = getpwuid(getuid());
    file_base_path = (char *)malloc(NAME_LEN * sizeof(char));
    if (!strcmp(pwd->pw_name, "root"))
    {
        sprintf(file_base_path, "/root");
    }
    else
    {
        sprintf(file_base_path, "/home/%s", pwd->pw_name);
    }
    server_sock = start_server();
    close(server_sock);
    return 0;
}