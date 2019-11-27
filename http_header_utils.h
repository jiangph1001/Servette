#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>   
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define HTTP_HEADER_NAME_LEN 30
#define HTTP_HEADER_CONTENT_LEN 200
// 请求头首部行结构体
// name表示此首部行的名字
// content表示此首部行的内容
// 对于请求头的第一个首部行也就是方法首部行
// 比如 "GET / HTTP/1.1", name字段是请求方法
typedef struct __http_header
{
    char name[HTTP_HEADER_NAME_LEN];
    char content[HTTP_HEADER_CONTENT_LEN];
    struct __http_header * next;
} _http_header,* http_header;

//请求头首部行链表结构
typedef struct __http_header_chain
{
    http_header first_header;
    int num_of_header;
}_http_header_chain, * http_header_chain;

/*
Description:
    获得所有http首部行链表
Return:
    http报文可能存在的内容部分的位置
*/
int get_http_headers(const char * buffer, http_header_chain *headers)
{
    // 第一条http首部行
    http_header http_header_first = NULL;
    // 用于构建http_header链表的中间量
    http_header temp_header = NULL;
    // 首部行总量
    int num_of_http_header = 0;
    // 当前在buffer中扫描的位置
    int pos_of_buffer = 0;
    // 下一条首部行在buffer中的位置
    int next_http_header_begin_pos = 0;

    int len_of_buffer = strlen(buffer);

    //获得首部行组成的链表
    while(pos_of_buffer < len_of_buffer)
    {
        // 此条件说明检测到了一行首部
        if (buffer[pos_of_buffer] == '\r' && buffer[pos_of_buffer + 1] == '\n')
        {
            // 首部行数量加一
            num_of_http_header = num_of_http_header + 1;

            //为新的首部行创建空间
            if(http_header_first == NULL)
            {
                http_header_first = (http_header)malloc(sizeof(_http_header));
                temp_header = http_header_first;
                temp_header->next = NULL;
            }
            else
            {
                temp_header->next = (http_header)malloc(sizeof(_http_header));
                temp_header = temp_header->next;
                temp_header->next =NULL;
            }
            //在temp_header中放入数据，temp_pos作为中间量
            int temp_pos = 0;

            // 首先获得首部行的名字
            for( ; temp_pos < HTTP_HEADER_NAME_LEN; )
            {
                if(buffer[next_http_header_begin_pos] == ' ' || buffer[next_http_header_begin_pos] == ':')
                    break;
                else
                    temp_header->name[temp_pos++] = buffer[next_http_header_begin_pos++];
            }
            temp_header->name[temp_pos] = '\0';
            // 名字之后是": "(或者是' ',这种情况仅限于第一行请求首部行)，再之后就是内容了
            if(buffer[next_http_header_begin_pos] == ' ')
                next_http_header_begin_pos = next_http_header_begin_pos + 1;
            else 
                next_http_header_begin_pos = next_http_header_begin_pos + 2;


            temp_pos = 0;
            // 再获得首部行的内容，
            for( ; temp_pos < HTTP_HEADER_CONTENT_LEN; )
            {
                if(buffer[next_http_header_begin_pos] == '\r')
                    break;
                else
                    temp_header->content[temp_pos++] = buffer[next_http_header_begin_pos++];
            }
            temp_header->content[temp_pos] = '\0';


            if (    // 此条件说明首部行结束了
                    buffer[pos_of_buffer + 2] == '\r' && buffer[pos_of_buffer + 3] == '\n'
            )
            {
                // 首部行结束了，之后是可能存在的(POST方法有)内容部分
                pos_of_buffer = pos_of_buffer + 4;
                break;
            }
            else
            {
                // 此条件说明接下来还有首部行
                pos_of_buffer =  pos_of_buffer + 2;
                next_http_header_begin_pos = pos_of_buffer;
            }
        
        }
        else
            pos_of_buffer = pos_of_buffer + 1;
    }
    (*headers)->first_header = http_header_first;
    (*headers)->num_of_header = num_of_http_header;
    return pos_of_buffer;
}

/*
Description:
    释放headers所指向的首部行链表占用的空间
Return:
    void
*/
void delete_http_headers(http_header_chain *headers)
{
    //释放首部行的空间
    http_header temp_header = (*headers)->first_header;
    http_header temp_header2 = NULL;
    while(temp_header)
    {
        //这里把temp_header变量当作了中间量
        temp_header2 = temp_header;
        temp_header = temp_header -> next;
        free(temp_header2);
    }
    free((*headers));
}

/*
Description:
    获得特定name首部行，对应的content
    第一个参数是首部行name,第二个参数是
    首部行content，第三个参数是首部行
    链表，最后是大小限制参数
Return:
    返回1表示找到，返回0表示没找到
*/
int get_http_header_content(const char * name, char * content, http_header_chain *headers, int limit)
{
        http_header temp_header = (*headers)->first_header;
        // 遍历首部行
        while(temp_header)
        {
            if( !strcmp(name, temp_header->name) )
            {
                strncpy(content, temp_header->content, strlen(temp_header->content));
                content[strlen(temp_header->content)] = '\0';
                return 1;
            }
            temp_header = temp_header -> next;
        }
        return 0;
}

/*
Description:
    headers参数是一个首部行链表，
    函数作用是打印所有的首部行
Return:
    void
*/
void print_http_headers(http_header_chain *headers)
{
        http_header temp_header = (*headers)->first_header;
        // 遍历首部行
        while(temp_header)
        {
            printf("%s %s\n", temp_header->name, temp_header->content);
            temp_header = temp_header -> next;
        }
}