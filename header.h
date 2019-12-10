#include <stdio.h>
#include "kmp.h"

/*
Description:
    根据读取的文件名的后缀，决定响应的Content-Type
Parameters:
    char *file: [IN] 解析的html文件名
Return:
    Content-Type
*/
const char *get_type_by_name(char *file)
{
    int len = strlen(file);
    if (kmp(file, ".htm", len) != -1)
    {
        return "text/html";
    }
    else if (kmp(file, ".jpg", len) != -1)
    {
        return "image/jpeg";
    }
    else if(kmp(file, ".ico", len) != -1)
    {
        return "images/x-icon";
    }
    else if(kmp(file, ".mp4", len) != -1)
    {
        return "video/mpeg4";
    }
    else if(!kmp(file, ".css", len) != -1)
    {
        return "text/css";
    }
    else
    {
        printf("unkown \n");
        //对于不识别的后缀格式，默认给text/html
        return "text/html";
    }
    
}

/*
Description:
    根据状态码，获取状态信息的描述
Parameters:
    int status [IN] 状态码
Return:
    状态信息
*/
const char *get_status_by_code(int status)
{
    switch (status)
    {
    // 1×× Informational
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing";
    // 2×× Success
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content";
    case 207:
        return "Multi-Status";
    case 208:
        return "Already Reported";
    case 226:
        return "IM Used";
    // 3×× Redirection
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy";
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permanent Redirect";
    // 4×× Client Error
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Payload Too Large";
    case 414:
        return "Request-URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Requested Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 418:
        return "I'm a teapot";
    case 421:
        return "Misdirected Request";
    case 422:
        return "Unprocessable Entity";
    case 423:
        return "Locked";
    case 424:
        return "Failed Dependency";
    case 426:
        return "Upgrade Required";
    case 428:
        return "Precondition Required";
    case 429:
        return "Too Many Requests";
    case 431:
        return "Request Header Fields Too Large";
    case 444:
        return "Connection Closed Without Response";
    case 451:
        return "Unavailable For Legal Reasons";
    case 499:
        return "Client Closed Request";
    // 5×× Server Error
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    case 506:
        return "Variant Also Negotiates";
    case 507:
        return "Insufficient Storage";
    case 508:
        return "Loop Detected";
    case 510:
        return "Not Extended";
    case 511:
        return "Network Authentication Required";
    case 599:
        return "Network Connect Timeout Error";
    default:
        return "Unknown HTTP status code";
    }
}

/*
Description:
    生成响应头
Parameters:
    char *header [OUT] 输出的响应头
    int status [IN] 状态码
    const char *type [IN] 内容类型
Return:
    NULL
*/
void construct_header(char *header, int status, const char *type)
{
    const char *msg = get_status_by_code(status);
    sprintf(header, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(header, "%sContent-Type:%s\r\n", header, type);
    sprintf(header, "%sServer:servette-UCAS\r\n", header);
    sprintf(header, "%sConnection: keep-alive\r\n", header);
    #ifdef _DEBUG
    printf("Response:\n--------\n%s\n----------\n", header);
    #endif
    sprintf(header, "%s\r\n", header);

}

/*
Description:
    生成响应头for Download，chunk
Parameters:
    char *header [OUT] 输出的响应头
    int status [IN] 状态码
    const char *file_name [IN] 下载的文件名
Return:
    NULL
*/
void construct_download_header(char *header, int status, const char *file_name)
{
    const char *msg = get_status_by_code(status);
    sprintf(header, "HTTP/1.1 %d %s\r\n", status, msg);
    //Content-Type:application/octet-stream为下载类型
    sprintf(header, "%sContent-Type:application/octet-stream\r\n", header);
    sprintf(header, "%sContent-Disposition: attachment;filename=%s\r\n", header, file_name);
    sprintf(header, "%sTransfer-Encoding:chunked\r\n", header); //下载文件通过chunk传输

    #ifdef _DEBUG
    printf("Response:\n--------\n%s\n----------\n", header);
    #endif
    sprintf(header, "%s\r\n", header);

}
