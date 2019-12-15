// 此头文件用于处理响应
//#include <string.h>
//#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
//#include <bits/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#include "config.h"
#include "header.h"
#include "urldecode.h"
#include "http_header_utils.h"
#include "ssl_utils.h"

// 上传下载文件夹位置
extern char *file_base_path;


/*
Description:
    判断socket是否关闭
Return:
    0：开启
    1：关闭
*/
int judge_socket_closed(int client_socket)
{
    char buff[32];
    int recvBytes = recv(client_socket, buff, sizeof(buff), MSG_PEEK);
    int sockErr = errno;

    if (recvBytes > 0) //Get data
        return 0;

    if ((recvBytes == -1) && (sockErr == EWOULDBLOCK)) //No receive data
        return 0;

    return 1;
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
void response_webpage(SSL * client_ssl, int client_sock, char *file)
{
    int fd, ret_code;
    unsigned long real_size;
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
    //读取文件
    fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        //打开文件失败时构造404的相应
        construct_header(header, 404, "text/html");
        
        // 这里暂时默认其real_write_size就是strlen(header)
        ret_code = ssl_write(client_ssl, header, strlen(header), &real_size);
        return;
    }
    else
    {
        const char *type = get_type_by_name(file_name);
        construct_header(header, 200, type);
        int ret_code = ssl_write(client_ssl, header, strlen(header), &real_size);
        //write(client_sock,echo_str,strlen(echo_str));
        while (size)
        {
            //size代表读取的字节数
            size = read(fd, buf, MAX_SIZE);
            if (size > 0)
            {
                // if(judge_socket_closed(client_sock))
                // {   
                //     printf("传输中断\n");
                //     return;
                // }
                ret_code = ssl_write(client_ssl, buf, size, &real_size);
            }
        }
        return;
    }
}

/*
Description:
    对于无法解析的请求，暂用作回声服务器
Parameters:
    int client_sock [IN] 客户端的socket
    char *msg: [IN] 消息内容
Return:
    NULL
*/
void response_echo(int client_sock, char *msg)
{
    write(client_sock, msg, strlen(msg));
}

/*
Description:
    实现简单的CGI功能,真实的CGI复杂得多
    处理列出文件列表的请求/?cgi-bin=[一个目录]
Parameters:
    int client_sock [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_cgi(SSL * client_ssl, int client_sock, char *arg)
{
    int size = -1, ret_code;
    unsigned long real_size;
    char html[MAX_SIZE], header[MAX_SIZE];
    char dir_name[NAME_LEN];
    if (sscanf(arg, "/?cgi-bin=%s", dir_name) == EOF)
    {
        //匹配失败！
        printf("error:%s\n", arg);
        construct_header(header, 404, "text/html");
        ret_code = ssl_write(client_ssl, header, strlen(header), &real_size);
        return;
    }
    else
    {
        //调用CGI程序，完成文件列表显示页面的构造
        char cmd[MIDDLE_SIZE];
        strncpy(cmd, "cgi-bin/filelist ", sizeof(cmd) - 1);
        strncat(cmd, dir_name, sizeof(cmd) - strlen(cmd) - 1);
        strncat(cmd, " ", sizeof(cmd) - strlen(cmd) - 1);
        strncat(cmd, file_base_path, sizeof(cmd) - strlen(cmd) - 1);
        FILE *fp = popen(cmd, "r");
        if (!fp)
        {
            //匹配失败！
            printf("error:%s\n", arg);
            construct_header(header, 404, "text/html");
            ret_code = ssl_write(client_ssl, header, strlen(header), &real_size);
            return;
        }
        //response_webpage(client_sock,"/");
        construct_header(header, 200, "text/html");
        ret_code = ssl_write(client_ssl, header, strlen(header), &real_size);
        while (fgets(html, sizeof(html), fp) != NULL)
        {
            // if(judge_socket_closed(client_sock))
            // {
            //     printf("传输中断\n");
            //     return;
            // }
            ret_code = ssl_write(client_ssl, html, strlen(html) - 1, &real_size);
            //注意sizeof("\r\n") == 3，算上了结尾的'\0'
            ret_code = ssl_write(client_ssl, "\r\n", sizeof("\r\n") - 1, &real_size);
        }
        pclose(fp);
    }
}

/*
Description:
    处理下载文件的请求/?download=[文件名]
    目前暂时弃用此函数
Parameters:
    int client_sock [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_download(int client_sock, char *arg)
{
    int fd;
    int size = -1;
    char buf[MAX_SIZE], header[MAX_SIZE];
    char file_name[NAME_LEN];
    if (sscanf(arg, "/?download=%s", file_name) == EOF)
    {
        //匹配失败！
        printf("error:%s\n", arg);
        construct_header(header, 404, "text/html");
        write(client_sock, header, strlen(header));
        return;
    }
    fd = open(file_name, O_RDONLY);
    if (fd == -1)
    {
        construct_header(header, 404, "text/html");
        write(client_sock, header, strlen(header));
        return;
    }
    //构建下载的相应头部
    printf("\tdownloading %s\n", file_name);
    construct_download_header(header, 200, file_name);
    write(client_sock, header, strlen(header));
    while (size)
    {
        //size代表读取的字节数
        size = read(fd, buf, MAX_SIZE);
        if (size > 0)
        {
            if(judge_socket_closed(client_sock))
            {
                printf("传输中断\n");
                return;
            }
            send(client_sock, buf, size, 0);
        }
    }
    printf("\n");
}

/*
Description:
    处理下载文件的请求/?download=[文件名]
    以chunk分块传输
Parameters:
    int client_sock [IN] 客户端的socket
    char *arg: [IN] 解析的参数
Return:
    NULL
*/
void response_download_chunk(SSL * ssl, int client_sock, char *arg)
{
    int fd;
    ssize_t size = -1;
    unsigned long real_size;
    int ret_code;
    char buf[MAX_SIZE], header[MAX_SIZE], *chunk_head;
    char file_name[NAME_LEN];
    if (sscanf(arg, "/?download=%s", file_name) == EOF)
    {
        //匹配失败！
        printf("error:%s\n", arg);
        construct_header(header, 404, "text/html");
        ret_code = ssl_write(ssl, header, strlen(header), &real_size);
        return;
    }
    fd = open(file_name, O_RDONLY);
    // FILE *fp = fopen(file_name,"rb");
    // if(fp == NULL)
    // {
    //     construct_header(header, 404, "text/html");
    //      write(client_sock, header, strlen(header));
    // }
    if (fd == -1)
    {
        construct_header(header, 404, "text/html");
        write(client_sock, header, strlen(header));
        ret_code = ssl_write(ssl, header, strlen(header), &real_size);
        return;
    }
    //构建下载的相应头部
    printf("\tdownloading %s\n", file_name);
    construct_download_header(header, 200, file_name);
    ret_code = ssl_write(ssl, header, strlen(header), &real_size);

    // while(fgets(buf,MAX_SIZE,fp) != NULL)
    // {
    //     size = read(fd, buf, MAX_SIZE);
    //     chunk_head = (char *)malloc(MIN_SIZE * sizeof(char));
    //     sprintf(chunk_head, "%x\r\n", size); //需要转换为16进制
    //     send(client_sock, chunk_head, strlen(chunk_head), 0);
    //     if (size > 0)
    //     {
    //         send(client_sock, buf, size, 0);
    //     }
    //     send(client_sock, CRLF, strlen(CRLF), 0);
    //     free(chunk_head);
    // }
    int flag = 1;
    while (size)
    {
        //size代表读取的字节数
        if (flag < 20)
        {
            usleep(350000);
            flag++;
        }
        else
            usleep(50000);
        size = read(fd, buf, MAX_SIZE);
#ifdef _DEBUG
        printf("读取了%dBytes\n", size);
#endif
        chunk_head = (char *)malloc(MIN_SIZE * sizeof(char));
        sprintf(chunk_head, "%x\r\n", size); //需要转换为16进制
        ret_code = ssl_write(ssl, chunk_head, strlen(chunk_head), &real_size);
        if (size > 0)
        {
            // if(judge_socket_closed(client_sock))
            // {
            //     printf("传输中断\n");
            //     return;
            // }
            ret_code = ssl_write(ssl, buf, size, &real_size);
        }
        ret_code = ssl_write(ssl, CRLF, strlen(CRLF), &real_size);
        free(chunk_head);
    }
    printf("下载完成\n");
    ret_code = ssl_write(ssl, CRLF, strlen(CRLF), &real_size);
    printf("\n");
}

/*
Description:
    从输入的buffer字符数组中的下标pos开始，到下一个"\r\n"。
    获取完整的一行。输入的数字代表限制长度。此函数仅仅可以
    在
Parameters:

Return:
    返回下一行第一个字符的位置
*/
int get_next_line(char *temp, const char *buffer, int pos_of_buffer, int limit)
{
    int pos_of_temp = 0;
    for (; pos_of_temp < limit;)
    {
        if (buffer[pos_of_buffer] == '\r' || buffer[pos_of_buffer + 1] == '\n')
            break;
        else
            temp[pos_of_temp++] = buffer[pos_of_buffer++];
    }
    temp[pos_of_temp] = '\0';

    // 这里还需要处理！！！
    return (pos_of_buffer + 2);
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
int upload_file(SSL * client_ssl, int client_sock, char *buffer, char *arg, http_header_chain headers, int begin_pos_of_http_content, int size_of_buffer)
{
    int content_length = 0;
    int temp_pos = 0;
    char temp[MAX_SIZE];
    char boundary[MIDDLE_SIZE];
    char filename[MIDDLE_SIZE];
    char *char_pointer;
    int ret_code;
    unsigned long real_size;

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
        response_cgi(client_ssl, client_sock, temp);
        return 0;
    }

    char_pointer = char_pointer + 9;
    while ((*char_pointer) != '\r')
    {
        boundary[temp_pos++] = (*char_pointer);
        char_pointer = char_pointer + 1;
    }
    boundary[temp_pos] = '\0';
    // boundary之前还会多两个"-"
    strncpy(temp, boundary, sizeof(temp) - 1);
    temp[strlen(boundary)] = '\0';
    strncpy(boundary, "--", sizeof(boundary) - 1);
    boundary[2] = '\0';
    strncat(boundary, temp, sizeof(boundary) - strlen(boundary) - 1);
    boundary[strlen(temp)] = '\0';
    printf("boundary: %s \n", boundary);

    //begin_pos_of_http_content很重要, HTTP数据包在这一点之后就是http数据包的实体部分
    //使用pos_of_buffer来显示现在读取的buffer的位置
    int pos_of_buffer = begin_pos_of_http_content;
    // 一次写入文件的量
    int written_size = 0;

    // 说明这里需要重新读数据了
    if( pos_of_buffer >= size_of_buffer )
    {
        ret_code = ssl_read(client_ssl, buffer, MAX_SIZE, &real_size);
        size_of_buffer = real_size;
        pos_of_buffer = 0;
    }

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
        strncpy(temp, "/?cgi-bin=", sizeof(temp));
        strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
        response_cgi(client_ssl, client_sock, temp);
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
        //这个文件已经存在了，就给文件名前面加上"new_"

        //strncpy(temp, "new_", sizeof(temp) - 1);
        //strncat(temp, filename, sizeof(temp) - strlen(temp) - 1);
        //strncpy(filename, temp, sizeof(filename) - 1);
        //strncpy(temp, arg, sizeof(temp) - 1);
        //strncat(temp, "/", sizeof(temp) - strlen(temp) - 1);
        //strncat(temp, filename, sizeof(temp) - strlen(temp) - 1);

        //这个文件已经存在了，就不让上传了
        strncpy(temp, "/?cgi-bin=", sizeof(temp));
        strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
        response_cgi(client_ssl, client_sock, temp);
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
                // if(judge_socket_closed(client_sock))
                // {
                //     printf("传输中断\n");
                //     return;
                // }
                // size_of_buffer = read(client_sock, buffer, MAX_SIZE);

                ret_code = ssl_read(client_ssl, buffer, MAX_SIZE, &real_size);
                size_of_buffer = real_size;
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
                //修改文件权限
                chmod(filename, 666);
                break;
            }
        }
        if (size_of_buffer <= 0)
            break;
    }

    //重新显示这个页面
    strncpy(temp, "/?cgi-bin=", sizeof(temp));
    strncat(temp, arg, sizeof(temp) - strlen(temp) - 1);
    response_cgi(client_ssl, client_sock, temp);
    return 0;
}

/*
Description:
    从客户端读取请求并对请求的内容进行处理
    分割为GET和POST请求
Parameters:
    void *p_client_sock [IN] 客户端的socket
Return:
    NULL
*/
void *do_Method(void *paras)
{
    //令当前线程分离
    //pthread_detach(pthread_self());

    // 从主函数中获取参数
    int client_sock = (*(p_do_method_para *)paras)->client_sock;
    SSL * client_ssl = (*(p_do_method_para *)paras)->client_ssl;
    int ret_code;
    unsigned long size_of_buffer;

    //int tid = pthread_self();
    // int break_cnt = 0; 用于指定tcp连接断开,一定次数的无连接即断开tcp
    char *Connection;  //记录Connection的连接信息
    char methods[5]; //GET or POST
    char *buffer, *message;
    buffer = (char *)malloc(MAX_SIZE * sizeof(char));
    message = (char *)malloc(MIDDLE_SIZE * sizeof(char));

    // 设置非阻塞
    // int flags;
    // flags = fcntl(client_sock, F_GETFL, 0);
    // flags |= O_NONBLOCK;
    // fcntl(client_sock, F_SETFL, flags);

    // 进行ssl握手
    // openssl库中的函数只有返回大于0才是正确执行的，否则都会有错误
    ret_code = SSL_accept(client_ssl);
    if(ret_code == 1)
    {
        printf("ssl握手成功\n");
    }
    else
    {
        handle_ssl_error(client_ssl, ret_code);
        goto Exit;
    }


    // while(1)
    // {
    //     if(judge_socket_closed(client_sock))
    //     {
    //         printf("客户端断开连接\n");
    //         //printf("tid:%d\n", tid);
    //         return;
    //     }
    //     if(ssl_accept(client_ssl) != 1)
    //     {
    //         int err_info = -1;
    //         int err = ssl_get_error(client_ssl, err_info);
    //         if( (err == ssl_error_want_read) || (err == ssl_error_want_write) )
    //         {
    //             usleep(30000);
    //             continue;
    //         }
    //         else
    //         {

    //             // ssl握手失败
    //             printf("ssl握手失败\n");

    //             ssl_shutdown(client_ssl);
    //             //ssl_free(client_ssl);

    //             close(client_sock);
    //             //free((*(p_do_method_para *)paras));
    //             return;
    //         }
    //     }
    //     else
    //     {
    //         // ssl握手成功, 可以开始接收数据了
    //         printf("ssl握手成功\n");
    //         break;
    //     }
    // }


        // break_cnt++;
        // if (break_cnt > BREAK_CNT)
        // {
        //     break;
        // }



        ret_code = ssl_read(client_ssl, buffer, MAX_SIZE, &size_of_buffer);
        if(ret_code == 0)
        {
            goto Exit;
        }
        else
        {
            if(size_of_buffer < MAX_SIZE)
            {
                // 避免输出buffer时乱码
                buffer[size_of_buffer] = '\0';
            }
        }


#ifdef _DEBUG
        //printf("tid:%d size:%d\n", tid, size_of_buffer);
        printf("size:%d\n", size_of_buffer);
        printf("Ruquest:\n--------\n%s\n--------\n", buffer);
#endif

        // break_cnt = 0;
        // buffer是接收到的请求，需要处理
        // 从buffer中分离出请求的方法和请求的参数
        int s_ret = sscanf(buffer, "%s %s", methods, message);
        if (s_ret != 2)
        {
            //如果buffer中的数据无法被分割为两个字符串，则默认echo
            response_echo(client_sock, buffer);
        }
        
        //获得所有的首部行组成的链表
        http_header_chain headers = (http_header_chain)malloc(sizeof(_http_header_chain));
        //begin_pos_of_http_content是buffer中可能存在的HTTP内容部分的起始位置, GET报文是没有的，POST报文有
        int begin_pos_of_http_content = get_http_headers(buffer, &headers);
        //print_http_headers(&headers);
        // Connection = (char *)malloc(MIN_SIZE * sizeof(char));
        // int keep_alive = get_http_header_content("Connection", Connection, &headers, MAX_SIZE);

        decode_message(message);

        switch (methods[0])
        {
        // GET
        case 'G':
            // 当message字符串开始就出现"/?download="子串时
            if (kmp(message, "/?download=", strlen(message)) == 0)
            {
                response_download_chunk(client_ssl, client_sock, message);
            }
            else if (kmp(message, "/?cgi-bin=", strlen(message)) == 0)
            // 当message字符串开始就出现"/?cgi-bin="子串时
            {
                response_cgi(client_ssl, client_sock, message);
            }
            else
            {
                response_webpage(client_ssl, client_sock, message);
            }
            break;
            // POST
        case 'P':
            upload_file(client_ssl, client_sock, buffer, message, headers, begin_pos_of_http_content, size_of_buffer);
            break;
        default:
            printf("不支持的请求:\n");
            response_echo(client_sock, buffer);
        }

#ifdef _DEBUG2
        if (!keep_alive)
        {
            printf("testConnection:%s\n", Connection);
        }
#endif
    

    Exit:
    free(buffer);
    free((*(p_do_method_para *)paras));
    free(message);
    SSL_free(client_ssl);
    close(client_sock);
}
