// 对url进行解码
#include <string.h>



/*
Description:
    将hex字符转换成对应的整数
Parameters:
    char c [IN] 要转换的字符
Return 
    0~15：转换成功，
    -1:表示c 不是 hexchar
 */
int hexchar2int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    else
        return -1;
}

/*
Description:
    对src进行url解码
Parameters:
    char *src [IN] urlencode后的字符串形式
Return 
    null: 字符串src的形式不对
    否则 解析成功后的字符串
 */
char *urldecode(char *src)
{
    int len = strlen(src);
    int count = len;
    char *dst = (char *)malloc(sizeof(char) * (count+1));
    if (! dst ) // 分配空间失败
        return NULL;
    //节约空间，直接用变量len和count来充当临时变量
    char *dst1 = dst;
    while(*src){//字符串没有结束
        if ( *src == '%')
        {//进入解析状态
            src++;
            len = hexchar2int(*src);
            src++;
            count = hexchar2int(*src);
            if (count == -1 || len == -1)
            {//判断字符转换成的整数是否有效
                *dst1++ = *(src-2);
                src--;
                continue;
            }
            *dst1++ =(char)( (len << 4) + count);//存储到目的字符串
        }
        else
        {
            *dst1++ = *src;
        }
        src++;
    }
    *dst1 = '\0';
    return dst;
}

/*
Description:
    对message进行url解码，直接修改message
Parameters:
    char *message [IN]/[OUT] 解码的字符串
Return 
    null
 */
void decode_message(char *message)
{
    char *decode_msg = urldecode(message);
    sprintf(message,"%s",decode_msg);
    free(decode_msg);
}
