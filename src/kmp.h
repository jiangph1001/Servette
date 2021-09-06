#include <string.h>

/*
Description:
    构造KMP算法中模式串的next数组
Parameters:
    const char * pat [IN] 模式串
    int * next [IN] 模式串的next数组
Return:
    void
*/
void get_next_of_pat(const char *pat, int *next)
{
    next[0] = -1;
    int i = 0, j = -1, size_of_pat = strlen(pat);

    while (i < size_of_pat)
    {
        if (j == -1 || pat[i] == pat[j])
        {
            ++i;
            ++j;
            next[i] = j;
        }
        else
        {
            j = next[j];
        }
    }
}

/*
Description:
    KMP算法
Parameters:
    const char * str [IN] 被查找的主串，可以是二进制形式的
    const char * pat [IN] 要查找的模式串，非二进制形式
    int size_of_str [IN] 主串的长度
Return:
    存在，返回模式串在字串中的位置
    不存在，返回-1
*/
int kmp(const char *str, const char *pat, int size_of_str)
{
    int i = 0, j = 0, size_of_pat = strlen(pat);
    int next[MIDDLE_SIZE];
    get_next_of_pat(pat, next);

    while (i < size_of_str && j < size_of_pat)
    {
        if (j == -1 || str[i] == pat[j])
        {
            ++i;
            ++j;
        }
        else
        {
            j = next[j];
        }
    }
    if (j == size_of_pat)
    {
        return i - j;
    }
    else
    {
        return -1;
    }
}