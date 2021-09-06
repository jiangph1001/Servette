#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../config.h"

void output_headers()
{
    printf("<!DOCTYPE>\n");
    printf("<html>\n");
    printf("<head> <meta charset=\"UTF-8\"> </head>\n");
    printf("<title>Servette</title>\n");
    printf("<body>\n");
    printf("<h1> Servette </h1>\n");
    printf("<h2> http文件服务器 </h2>\n");
    printf("<hr/ >\n");
}

/*
Description:
    生成错误提示的html代码
Parameters:
    char *file_base_path [IN] 用户可以访问的根目录
    char *dir_path [IN] 用户请求的位置
    char *error_code [IN] 提示用户错误发生的原因
Return:
    NULL
*/
void output_error_html(char *file_base_path, char *dir_path, char *error_code)
{
    // 输出HTML
    // 输出错误信息和提示
    printf("<p> 无法访问 %s</p>\n", dir_path);
    printf("<p> %s </p>\n", error_code);
    printf("<a href=\"/?cgi-bin=%s\">点此返回%s</a>\n", file_base_path, file_base_path);

    printf("<hr/ >\n");
    printf("</body>\n");
    printf("</html>\n");
}

/*
Description:
    生成显示Linux中一个目录下的目录和文件的html代码
Parameters:
    char *file_base_path [IN] 用户可以访问的根目录
    char *dir_path [IN] 用户请求的位置
Return:
    NULL
*/
void output_files_and_dirs(char *file_base_path, char *dir_path)
{
    DIR *dir_info = opendir(dir_path);
    if (dir_info == NULL)
    {
        // 打开目录文件失败
        output_error_html(file_base_path, dir_path, "This is a folder. But cannot open this folder");
        return;
    }
    else
    {
        // 设置一些需要用到的数据和结构体
        struct dirent *file_in_dir;
        char file_path[MIDDLE_SIZE];
        char father_path[MIDDLE_SIZE];
        struct stat file_info;

        // 输出HTML
        output_headers();

        // action这里填入当前显示的目录页面
        printf("<form action = \"%s\" method=\"post\" enctype=\"multipart/form-data\">\n", dir_path);
        printf("<label for=\"file\">文件名：</label>\n");
        printf("<input type=\"file\" name=\"file\" id=\"file\" />\n");
        printf("<br />\n");
        printf("<p><input type=\"submit\" value=\"上传\" /></p>\n");
        printf("</form>\n");
        printf("<hr />\n");
        printf("<pre>\n");
        printf("<table frame=\"void\"  width=\"50%\" style=\"text-align:left\">\n");
        // 如果当前要显示的目录dir_path,不是用户可以访问的跟目录file_base_path，那么可以显示上一级目录
        if (strcmp(dir_path, file_base_path) != 0)
        {
            // Linux中的目录以"/"分割，这里去掉最后一个"/"后的内容即可
            char *char_pointer_of_end = strrchr(dir_path, '/');
            char *char_pointer_of_dir_path = dir_path;
            char *char_pointer_of_father_path = father_path;
            while (char_pointer_of_dir_path != char_pointer_of_end)
            {
                *(char_pointer_of_father_path++) = *(char_pointer_of_dir_path++);
            }
            *char_pointer_of_father_path = '\0';
            printf("<tr>\n");
            printf("<th><a href=\"/?cgi-bin=%s\">上一级</a></th>\n", father_path);
            printf("<th>目录</th>\n");
            printf("</tr>\n");
        }

        // 循环读取目录中文件的内容
        while (file_in_dir = readdir(dir_info))
        {
            if ((strcmp(file_in_dir->d_name, ".") == 0) || (strcmp(file_in_dir->d_name, "..") == 0))
            {
                continue;
            }
            strncpy(file_path, dir_path, sizeof(file_path) - 1);
            strncat(file_path, "/", sizeof(file_path) - strlen(file_path) - 1);
            strncat(file_path, file_in_dir->d_name, sizeof(file_path) - strlen(file_path) - 1);

            stat(file_path, &file_info);

            if (S_ISDIR(file_info.st_mode))
            {
                // 是目录
                printf("<tr>\n");
                printf("<th>><a href=\"/?cgi-bin=%s\">%s</a></th>", file_path, file_in_dir->d_name);
                printf("<th>目录</th>\n");
                printf("</tr>\n");
            }
            else
            {
                // 是文件
                printf("<tr>\n");
                printf("<th>&nbsp;<a href=\"/?download=%s\" target=\"_blank\">%s</a></th>", file_path, file_in_dir->d_name);
                printf("<th>文件</th>\n");
                printf("</tr>\n");
            }
        }

        printf("</table>\n");
        printf("</pre>\n");
        printf("<hr />\n");
        printf("</body>\n");
        printf("</html>\n");
        closedir(dir_info);
    }
}

/*
Description:
    被popen函数调用
    执行动态生成html代码的功能
Parameters:
    传入三个参数
    第一个参数是此程序名字filelist
    第二个参数是用户请求的目录dir_path
    第三个参数是程序执行时规定的用户可以读取的根目录file_base_path
Return:
    返回0
*/
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("参数错误\n");
        return 0;
    }

    char *dir_path = argv[1];
    char *file_base_path = argv[2];

    // 检查dir_path目录是否在file_base_path目录下
    char *char_pointer = strstr(dir_path, file_base_path);
    if (char_pointer == NULL || char_pointer != dir_path)
    {
        output_headers();
        // 不在
        output_error_html(file_base_path, dir_path, "文件夹不存在或无法访问");
        return 0;
    }

    // 检查dir_path目录是否存在
    if (access(dir_path, F_OK) != 0)
    {
        output_headers();
        // 不存在
        output_error_html(file_base_path, dir_path, "文件夹不存在");
        return 0;
    }

    // 检查dir_path是否是文件夹
    struct stat file_info;
    // stat函数，把文件信息放入到struct stat file_info结构中
    stat(dir_path, &file_info);

    if (S_ISDIR(file_info.st_mode))
    {
        // 是目录
        output_files_and_dirs(file_base_path, dir_path);
    }
    else
    {
        // 是文件
        output_error_html(file_base_path, dir_path, "这不是文件夹，是一个文件");
    }
    return 0;
}