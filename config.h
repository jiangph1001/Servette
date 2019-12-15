#define HOST "0.0.0.0"
#define PORT 8080
#define MAX_SIZE 4096
#define MIDDLE_SIZE 256
#define BREAK_CNT 16
#define MIN_SIZE 16
#define NAME_LEN 64
#define DEFAULT_FILE "index.html"
#define CGISTR "cgi-bin/"
#define LOCALDIR "/"
#define HTML_DIR "www/"
#define MAX_THRAD 5
#define KEEPALIVE_TIMEOUT 3
#define _DEBUG
//#define _DEBUG2
#define FILE_DIR "/file"
#define SELECT_SOCKET 0
#define SELECT_LIBEVENT 1

#include <openssl/ssl.h>
#include <openssl/err.h>

const char *CRLF ="\r\n";
const unsigned int select_mode = SELECT_SOCKET;

// 这个数据结构用来向新开的线程传递参数
typedef struct do_method_para_st{
    int client_sock;
    SSL * client_ssl;
}* p_do_method_para;
