
#include <stdio.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>

#include <pthread.h>
#include <pwd.h>
#include "response.h"

char *file_base_path;

struct ub
{
    int flag;
    char buffer[MAX_SIZE];
} upload_buffer;
void socket_event_cb(struct bufferevent *bev, short events, void *arg);

char *big_buffer;
/*
Description:
    实现简单的CGI功能,真实的CGI复杂得多
    处理列出文件列表的请求/?cgi-bin=[一个目录]
Parameters:
    int bev [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_cgi_Event(struct bufferevent *bev, char *arg)
{
    int size = -1;
    char html[MAX_SIZE], header[MAX_SIZE];
    char dir_name[NAME_LEN];
    if (sscanf(arg, "?cgi-bin=%s", dir_name) == EOF)
    {
        //匹配失败！
        printf("error:%s\n", arg);
        construct_header(header, 404, "text/html");
        bufferevent_write(bev, header, strlen(header));
        return;
    }
    else
    {
        //调用CGI程序，完成文件列表显示页面的构造
        char cmd[MIDDLE_SIZE];
        sprintf(cmd, "cgi-bin/filelist %s %s", dir_name, file_base_path);
        printf("%s \n", cmd);
        FILE *fp = popen(cmd, "r");
        if (!fp)
        {
            //匹配失败！
            printf("error:%s\n", arg);
            construct_header(header, 404, "text/html");
            bufferevent_write(bev, header, strlen(header));
            return;
        }

        //response_webpage(bev,"/");
        construct_header(header, 200, "text/html");
        bufferevent_write(bev, header, strlen(header));

        while (fgets(html, sizeof(html), fp) != NULL)
        {
            bufferevent_write(bev, html, strlen(html) - 1);
            //注意sizeof("\r\n") == 3，算上了结尾的'\0'
            bufferevent_write(bev, "\r\n", sizeof("\r\n") - 1);
        }
        pclose(fp);
    }
}

int read_my_buffer(char *buffer)
{
    if (upload_buffer.flag)
    {
        //有数据
        sprintf(buffer, "%s", upload_buffer.buffer);
        upload_buffer.flag = 0;
        return strlen(upload_buffer.buffer);
    }
    else
    {
        usleep(5000);
        return -1;
    }
}

int write_my_buffer(char *buffer)
{
    if (upload_buffer.flag)
    {
        usleep(500);
        return -1;
    }
    else
    {
        sprintf(upload_buffer.buffer, "%s", buffer);
        upload_buffer.flag = 1;
        return 0;
    }
}

/*
Description:
    当post请求来临时，调用此函数
*/
void post_cb(struct bufferevent *bev, void *arg)
{
    static int size = 0;
    usleep(50000);
    printf("临时的回调函数\n");
}
/*
Description:
    从输入的buffer字符数组中的下标pos开始，到下一个"\r\n"。
    获取完整的一行。输入的数字代表限制长度。此函数仅仅可以
    在
Parameters:

Return:
    成功返回0，失败返回1
*/
int upload_file_Event(struct bufferevent *bev, char *buffer_, char *arg, http_header_chain headers, int begin_pos_of_http_content)
{
    char *buffer = big_buffer + 5;
    int content_length = 0;
    int temp_pos = 0;
    int size_of_buffer = strlen(buffer);
    char temp[MAX_SIZE];
    char boundary[MIDDLE_SIZE];
    char filename[MIDDLE_SIZE];
    char *char_pointer;

    //首先从首部行Content-Type中获得分隔符boundary
    //执行以下函数，temp数组中存储的是名字为Content-Type的首部行的，首部行内容
    get_http_header_content("Content-Type", temp, &headers, MAX_SIZE);
    // 提取出temp数组中的boundary
    char_pointer = strstr(temp, "boundary");

    // 这里可能空指针
    if (char_pointer == NULL)
    {
        strncpy(temp, "/?cgi-bin=", sizeof(temp));
        strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
        response_cgi_Event(bev, temp);
        return 0;
    }
    char_pointer = char_pointer + 9;
    while ((*char_pointer) != '\r')
    {
        boundary[temp_pos++] = (*char_pointer);
        char_pointer = char_pointer + 1;
    }
    boundary[temp_pos] = '\0';
    sprintf(temp, "%s", boundary);
    sprintf(boundary, "--%s", temp);

    //printf("boundary: %s \n", boundary);
    //begin_pos_of_http_content很重要, HTTP数据包在这一点之后就是http数据包的实体部分
    //使用pos_of_buffer来显示现在读取的buffer的位置
    int pos_of_buffer = begin_pos_of_http_content;
    // 一次写入文件的量
    int written_size = 0;

    //首先读取第一行，这里第一行是boundary
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    //假定浏览器发过来的POST请求都是关于上传文件的
    //那么第二行一定包含了文件名这一信息，
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);
    //从temp数组中提取文件名字
    temp_pos = 0;
    char_pointer = strstr(temp, "filename");

    // 这里可能空指针
    if (char_pointer == NULL)
    {
        strncpy(temp, "?cgi-bin=", sizeof(temp));
        strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
        response_cgi_Event(bev, temp);
        return 0;
    }

    char_pointer = char_pointer + 10;
    while ((*char_pointer) != '\"')
    {
        filename[temp_pos++] = (*char_pointer);
        char_pointer = char_pointer + 1;
    }
    filename[temp_pos] = '\0';
    strncpy(temp, arg, sizeof(temp) - 1);
    strncat(temp, "/", sizeof(temp) - strlen(temp) - 1);
    strncat(temp, filename, sizeof(temp) - strlen(temp) - 1);

    if (access(temp, F_OK) != -1)
    {
        //这个文件已经存在了，就不让上传了
        strncpy(temp, "?cgi-bin=", sizeof(temp));
        strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
        response_cgi_Event(bev, temp);
        return 0;
    }

    strncpy(filename, temp, sizeof(filename));

    //根据文件名，创建文件
    int fd = open(filename, O_CREAT | O_WRONLY);

    //第三行是文件类型信息
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);

    //第四行是空行
    pos_of_buffer = get_next_line(temp, buffer, pos_of_buffer, MAX_SIZE - 1);

    //从第5行开始时文件内容，直到遇见boundary结束
    while (1)
    {
        // 查找在buffer数组中pos_of_buffer位置之后是否存在
        // boundary，存在证明file结尾在此buffer中，不存在
        // 证明还需要再读socket中的数据
        int pos_of_boundary_after_pos_of_buffer = kmp(buffer + pos_of_buffer, boundary, size_of_buffer - pos_of_buffer);
        // file的结尾不存在于此buffer中
        if (pos_of_boundary_after_pos_of_buffer == -1)
        {
            written_size = write(fd, buffer + pos_of_buffer, size_of_buffer - pos_of_buffer);
            printf("写入的数量: %d\n", written_size);
            if (written_size == -1)
            {
                //删除文件并提示出错
                close(fd);
                unlink(filename);
                break;
            }
            else
            {
                //读socket中的数据
                usleep(300000);
                size_of_buffer = bufferevent_read(bev, buffer, MAX_SIZE);

                printf("从socket中读出的量：%d\n", size_of_buffer);
                pos_of_buffer = 0;
            }
        }
        else //file的结尾存在于此buffer中
        {
            written_size = write(fd, buffer + pos_of_buffer, pos_of_boundary_after_pos_of_buffer);
            printf("写入的数量: %d\n", written_size);
            if (written_size == -1)
            {
                //删除文件并提示出错
                close(fd);
                unlink(filename);
                break;
            }
            else
            {
                //关闭文件
                close(fd);
                break;
            }
        }
        if (size_of_buffer <= 0)
            break;
    }

    //重新显示这个页面
    strncpy(temp, "?cgi-bin=", sizeof(temp));
    strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
    response_cgi_Event(bev, temp);
    return 0;
}

/*
Description:
    对于无法解析的请求，暂用作回声服务器
Parameters:
    int bev [IN] 客户端的socket
    char *msg: [IN] 消息内容
Return:
    NULL
*/
void response_echo_Event(struct bufferevent *bev, char *msg)
{
    int cnt = 5;
    while (cnt--)
    {
        bufferevent_write(bev, msg, strlen(msg));
        usleep(500000);
        printf("%d\n", cnt);
    }
}

/*
Description:
    处理下载文件的请求/?download=[文件名]
    以chunk分块传输
Parameters:
    int bev [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_download_chunk_Event(struct bufferevent *bev, char *arg)
{
    int fd;
    ssize_t size = -1;
    char buf[MAX_SIZE], header[MAX_SIZE], *chunk_head;
    char file_name[NAME_LEN];
    if (sscanf(arg, "?download=%s", file_name) == EOF)
    {
        //匹配失败！
        printf("error:%s\n", arg);
        construct_header(header, 404, "text/html");
        bufferevent_write(bev, header, strlen(header));
        return;
    }
    fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        construct_header(header, 404, "text/html");
        bufferevent_write(bev, header, strlen(header));
        return;
    }
    //构建下载的相应头部
    printf("\tdownloading %s\n", file_name);
    construct_download_header(header, 200, file_name);
    bufferevent_write(bev, header, strlen(header));
    int flag = 1;
    while (size)
    {
        size = read(fd, buf, MAX_SIZE);
#ifdef _DEBUG
        printf("读取了%dBytes\n", size);
#endif
        chunk_head = (char *)malloc(MIN_SIZE * sizeof(char));
        sprintf(chunk_head, "%x\r\n", size); //需要转换为16进制
        bufferevent_write(bev, chunk_head, strlen(chunk_head));
        int ws_ret;
        if (size > 0)
        {
            ws_ret = bufferevent_write(bev, buf, size);
            printf("写入结果:%d\n", ws_ret);
        }
        bufferevent_write(bev, CRLF, strlen(CRLF));
        free(chunk_head);
    }
    printf("下载完成\n");
    bufferevent_write(bev, CRLF, strlen(CRLF));
    printf("\n");
}

/*
Description:
    读取html文件，并显示在网页上
Parameters:
    int bev [IN] 客户端的socket
    char *file: [IN] 解析的html文件名
Return:
    NULL
*/
void response_webpage_Event(struct bufferevent *bev, char *file)
{
    int fd;

    char buf[MAX_SIZE], header[MAX_SIZE];
    int size = -1;
    char *file_name;
    file_name = (char *)malloc(MIDDLE_SIZE * sizeof(char));
    if (strcmp(file, "/") == 0)
    {
        //如果没有指定文件，则默认打开index.html
        sprintf(file_name, "%s%s", HTML_DIR, DEFAULT_FILE);
    }
    else
    {
        sprintf(file_name, "%s%s", HTML_DIR, file);
    }
    int pos = kmp(file_name, "?", strlen(file_name));
    if (pos != -1)
    {
        file_name[pos] = '\0';
    }
    //读取文件
    fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        //打开文件失败时构造404的相应
        construct_header(header, 404, "text/html");
        bufferevent_write(bev, header, strlen(header));
        fd = open("www/err.html", O_RDONLY);
    }
    else
    {
        const char *type = get_type_by_name(file_name);
        construct_header(header, 200, type);
        bufferevent_write(bev, header, strlen(header));
    }
    while (size)
    {
        //size代表读取的字节数
        size = read(fd, buf, MAX_SIZE);
        if (size > 0)
        {
            //printf("写入了%d\n", size);
            bufferevent_write(bev, buf, size);
        }
    }
    bufferevent_write(bev, CRLF, strlen(CRLF));
}
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
    static int fd_list[100] = {0};
    static int big_buffer_size = 0;
    static int flag = 0;
    char *buffer;
    buffer = (char *)malloc(MAX_SIZE);
    int fd = *(int *)arg; //代表当前打开的socket的文件描述符

    size_t len = bufferevent_read(bev, buffer, strlen(buffer) - 1);
    printf("本次读了%d,还有%d字节需要读取\n", len, fd_list[fd]);
    buffer[len] = '\0';
    printf("fd_list: %d\n", fd_list[fd]);
    if (fd_list[fd] > 0)
    {
        fd_list[fd] -= len;
        printf("本次读了%d,还有%d字节需要读取\n", len, fd_list[fd]);
        if (fd_list[fd] <= 0)
        {
            fd_list[fd] = 0;
            buffer = big_buffer;
        }
        big_buffer = (char *)realloc(big_buffer, big_buffer_size + len);
        big_buffer_size += len;
        sprintf(big_buffer, "%s%s", big_buffer, buffer);
        printf("now big buffer is %d\n", big_buffer_size);
        return;
    }

    big_buffer_size = 0;

    printf("\n\nGet Ruquest:\n--------\n%s\n--------\n", buffer);
    //do_Method_Event(bev, buffer);
    char *methods, *message;
    message = (char *)malloc(MIDDLE_SIZE * sizeof(char));
    methods = (char *)malloc(MAX_SIZE * sizeof(char));
    //如果发送的请求特别的非法，比如第一个字符串长度超过了5，就会段错误，所以这里将methods长度增长
    int s_ret = sscanf(buffer, "%s %s", methods, message);
    if (s_ret != 2)
    {
        //如果buffer中的数据无法被分割为两个字符串，则默认echo
        response_echo_Event(bev, buffer);
        return;
    }

    //获得所有的首部行组成的链表
    http_header_chain headers = (http_header_chain)malloc(sizeof(_http_header_chain));
    //begin_pos_of_http_content是buffer中可能存在的HTTP内容部分的起始位置, GET报文是没有的，POST报文有
    int begin_pos_of_http_content = get_http_headers(buffer, &headers);

    char *ContentLength = (char *)malloc(MIN_SIZE * sizeof(char));
    int cl_ret = get_http_header_content("Content-Length", ContentLength, &headers, MAX_SIZE);
    if (cl_ret == 0)
    {
        printf("Content-Length:%s\n", ContentLength);
        int content_length = atoi(ContentLength);
        int remain = content_length - len;
        printf("读了%d,%d待读取\n", len, remain);
        if (remain < 0)
            remain = 0;
        fd_list[fd] = remain;
    }

    decode_message(message);
    int pos;
    switch (methods[0])
    {
    // GET
    case 'G':
        if ((pos = kmp(message, "?download=", strlen(message))) != -1)
        {
            response_download_chunk_Event(bev, message + pos);
        }
        else if ((pos = kmp(message, "?cgi-bin=", strlen(message))) != -1)
        // 当message字符串开始就出现"/?cgi-bin="子串时
        {
            response_cgi_Event(bev, message + pos);
        }
        else
        {
            response_webpage_Event(bev, message);
        }
        break;
    case 'P':
        big_buffer = (char *)realloc(big_buffer, big_buffer_size + len);
        big_buffer_size += len;
        sprintf(big_buffer, "%s", buffer);
        printf("now big buffer is %d\n", big_buffer_size);
        //bufferevent_setcb(bev,temp_cb,temp_cb,socket_event_cb,NULL);
        //upload_file_Event(bev, buffer, message, headers, begin_pos_of_http_content);
        break;
    default:
        printf("不支持的请求:\n");
        response_echo_Event(bev, buffer);
    }
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
void socket_event_cb(struct bufferevent *bev, short events, void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        printf("connection closed\n");
    }
    else if (events & BEV_EVENT_ERROR)
    {
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

    printf("accept a client %d\n", fd);
    int *p_fd;
    *p_fd = fd;
    struct event_base *base;
    struct bufferevent *bev;

    base = (struct event_base *)arg;
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    // bev->timeout_write.tv_sec = 1;
    struct timeval *t = (struct timeval *)malloc(sizeof(struct timeval));
    t->tv_sec = 3;
    t->tv_usec = 0;
    bufferevent_set_timeouts(bev, t, t); //设置写超时为3秒
    //bufferevent_setwatermark(bev,EV_READ ,MAX_SIZE - 1,MAX_SIZE);
    printf("写水位:%d %d\n", bev->wm_write.high, bev->wm_write.low);
    printf("读水位:%d %d\n", bev->wm_read.high, bev->wm_read.low);
    //rintf("写超时：%d %d\n", bev->timeout_write.tv_sec, bev->timeout_write.tv_usec);
    //第三个参数为write_cb，暂不设置
    //
    bufferevent_setcb(bev, socket_read_cb, socket_write_cb, socket_event_cb, p_fd);
    bufferevent_enable(bev, EV_READ);
}

int main(int argc, const char *argv[])
{
    struct event_base *base;
    struct sockaddr_in server_addr;

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
    base = event_base_new();
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

    struct evconnlistener *listener = evconnlistener_new_bind(base, listener_cb, base,
                                                              LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 20,
                                                              (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));

    event_base_dispatch(base);
    evconnlistener_free(listener);
    event_base_free(base);

    return 0;
}