#include "openssl/ssl.h"
#include "openssl/err.h"

// 这个数据结构用来向新开的线程传递参数
typedef struct do_method_para_st{
    int client_sock;
    SSL * client_ssl;
}* p_do_method_para;


SSL_CTX * init_ssl(char * cert_file, char * private_key_file)
{
    SSL_CTX * ctx;
    // 载入所有的加密算法
    OpenSSL_add_all_algorithms();
    // ssl库初始化
    SSL_library_init();
    // 载入所有的SSL错误消息
    ERR_load_crypto_strings();
    ERR_load_SSL_strings();
    SSL_load_error_strings();
    // 以 SSL V2 和 V3 标准兼容方式产生一个 SSL_CTX ，即 SSL Content Text
    // 也可以用 SSLv2_server_method() 或 SSLv3_server_method() 单独表示 V2 或 V3标准
    ctx = SSL_CTX_new(SSLv23_server_method());

    if(ctx == NULL)
    {
        ERR_print_errors_fp(stdout);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // 载入网站的数字证书， 此证书用来发送给浏览器。 证书里包含有网站的公钥
    if(SSL_CTX_use_certificate_file(ctx, "cert/Servette.crt", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stdout);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // 载入网站的私钥，用来与客户握手用
    if (SSL_CTX_use_PrivateKey_file(ctx, "cert/Servette.key", SSL_FILETYPE_PEM) <= 0) 
    {
        ERR_print_errors_fp(stdout);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // 检查网站的公钥私钥是否匹配
    if (!SSL_CTX_check_private_key(ctx))
    {
        ERR_print_errors_fp(stdout);
        SSL_CTX_free(ctx);
        return NULL;
    }

    // SSL数据加载成功
    return ctx;
}

// 简单的封装一下非阻塞的从ssl通道读数据
int servette_read(SSL * ssl, int socket, char * buffer, int buffer_size)
{
    int total_write = 0;
    int oneround_write = 0;
    int flag;
    while(1)
    {
        // SSL_read_ex返回值是1或0，1表示成功0表示失败。oneround_count存储成功时读到的字节数
        flag = SSL_read_ex(ssl, buffer + total_write, buffer_size - total_write, (size_t *)&oneround_write);
        int err = SSL_get_error(ssl, oneround_write);
        if(err == SSL_ERROR_NONE)
        {
            // SSL_ERROR_NONE，表示没有错误
            if( oneround_write > 0 )
            {
                // 收到至少一个字节的数据，那么继续读下去
                total_write = total_write + oneround_write;
                if(total_write >= buffer_size)
                {
                    // buffer已经满了
                    return total_write;
                }
                else
                {
                    // 继续读
                    continue;
                }
            }else if( oneround_write == 0 )
            {
                // 没有收到数据说明客户端当前可能并不在发送数据了
                return total_write;
            }
        }
        else if( err == SSL_ERROR_ZERO_RETURN )
        {
            // ssl通道已经断开，但socket连接可能没断开
            return -1;
        }
        else if( ( err == SSL_ERROR_WANT_READ ) || (err == SSL_ERROR_WANT_WRITE) )
        {
            // 这两个错误表示需要重新调用SSL_read_ex
            // 那就休眠一会了再read,其实这里最好还是用select
            usleep(30000);
            continue;
        }
        else
        {
            // 其他类型的错误，可能不会出现其他类型的错误了
            return -2;
        }
        
    }
}

// 简单的封装一下非阻塞的向ssl通道写数据
int servette_write(SSL * ssl, int socket, char * buffer, int write_size)
{
    int total_write = 0;
    int oneround_write = 0;
    int flag;
    while(1)
    {
        // SSL_read_ex返回值是1或0，1表示成功0表示失败。oneround_count存储成功时读到的字节数
        flag = SSL_write_ex(ssl, buffer + total_write, write_size - total_write, (size_t *)&oneround_write);
        int err = SSL_get_error(ssl, oneround_write);
        if(err == SSL_ERROR_NONE)
        {
            // SSL_ERROR_NONE，表示没有错误
            if( oneround_write >= 0 )
            {
                // 写入了一些数据
                total_write = total_write + oneround_write;
                if(total_write == write_size)
                {
                    // 已经写完了所有的数据
                    return total_write;
                }
                else
                {
                    // 继续写
                    continue;
                }
            }
        }
        else if( err == SSL_ERROR_ZERO_RETURN )
        {
            // ssl通道已经断开，但socket连接可能没断开
            return -1;
        }
        else if( ( err == SSL_ERROR_WANT_READ ) || (err == SSL_ERROR_WANT_WRITE) )
        {
            // 这两个错误表示需要重新调用SSL_write_ex
            // 那就休眠一会了再write,其实这里最好还是用select
            usleep(30000);
            continue;
        }
        else
        {
            // 其他类型的错误，可能不会出现其他类型的错误了
            return -2;
        }
        
    }
}

/*
Description:
    这个函数用来处理调用openssl库函数之后
    可能产生的SSL_ERROR,并输出错误信息
Parameters:
    SSL *ssl [IN] openssl库函数的操作对象
    int ret_code [IN] openssl库函数调用后的返回值
Return:
    NULL
*/
void handle_ssl_error(SSL * ssl, int ret_code)
{
    int err = SSL_get_error(ssl, ret_code);
    if(err == SSL_ERROR_WANT_ACCEPT)
    {
        printf("SSL_ERROR_WANT_ACCEPT\n");
    }
    else if(err == SSL_ERROR_WANT_CONNECT)
    {
        printf("SSL_ERROR_WANT_CONNECT\n");
    }
    else if(err == SSL_ERROR_ZERO_RETURN)
    {
        printf("SSL_ERROR_ZERO_RETURN\n");
    }
    else if(err == SSL_ERROR_NONE)
    {
        printf("SSL_ERROR_NONE\n");
    }
    else if(err == SSL_ERROR_SSL)
    {
        printf("SSL_ERROR_SSL\n");
        char msg[1024];
        unsigned long e = ERR_get_error();
        ERR_error_string_n(e, msg, sizeof(1024));
        printf("%s\n%s\n%s\n%s\n", msg, ERR_lib_error_string(e), ERR_func_error_string(e), ERR_reason_error_string(e));
    }
    else if(err == SSL_ERROR_SYSCALL)
    {
        printf("SSL_ERROR_SYSCALL\n");
        char msg[1024];
        unsigned long e = ERR_get_error();
        ERR_error_string_n(e, msg, sizeof(1024));
        printf("%s\n%s\n%s\n%s\n", msg, ERR_lib_error_string(e), ERR_func_error_string(e), ERR_reason_error_string(e));
    }
}

/*
Description:
    简单的封装了openSSL库中的SSL_read_ex
    函数。功能是从SSL通道中读取数据。
Parameters:
    SSL *ssl [IN] 要读取的ssl通道
    char * buffer [IN] 读出来的数据的存放位置
    int max_size_of_buffer [IN] 最多可读的数据量
    unsigned long * *size_of_buffer [IN] 实际读出来的数据量
Return:
    int [OUT] 1表示读成功,0表示读失败并需要关闭ssl通道
*/
int ssl_read(SSL * ssl, char * buffer, int max_size_of_buffer, unsigned long *size_of_buffer)
{
    // SSL_read_ex第四个参数是指针值
    // 表示读到的数据的多少。此函数
    // 返回值为1表示成功，0表示错误
    int ret_code = SSL_read_ex(ssl, buffer, max_size_of_buffer, size_of_buffer);
    if(ret_code == 1)
    {
        return 1;
    }
    else
    {
        handle_ssl_error(ssl, ret_code);
        return 0;
    }
}

/*
Description:
    简单的封装了openSSL库中的SSL_write_ex
    函数。功能是向SSL通道中写入数据。
Parameters:
    SSL *ssl [IN] 要写入数据的ssl通道
    char * buffer [IN] 要写入的数据的存放位置
    int max_size_of_buffer [IN] 要写入的数据的量
    int *size_of_buffer [IN] 实际写入的数据的量
Return:
    int [OUT] 1表示写成功,0表示写失败并需要关闭ssl通道
*/
int ssl_write(SSL * ssl, const char * buffer, int want_write, unsigned long *real_write)
{
    // SSL_write_ex第四个参数是指针值
    // 表示写入的数据的多少。此函数
    // 返回值为1表示成功，0表示错误
    int ret_code = SSL_write_ex(ssl, buffer, want_write, real_write);
    if(ret_code == 1)
    {
        return 1;
    }
    else
    {
        handle_ssl_error(ssl, ret_code);
        return 0;
    }
}