import os
import sys

# 输出错误位置的页面
def out_put_error_html(dir_path, error_code):
    print("<!DOCTYPE>")
    print("<html>")
    print("<head> <meta charset=\"UTF-8\"> </head>")
    print("<body>")
    print("<h1> Naive File Server </h1>")
    print("<hr/ >")

    # action这里填入当前文件夹
    print("<p>" + dir_path + "</p>")
    print("<p>" + error_code + "</p>")

    print("<hr/ >")
    print("</body>")
    print("</html>")    

# 在HTML中，输出dir_path下文件和文件夹访问或下载链接
# 并输出上传文件到此dir_path的form表单
def out_put_files_and_dirs(dir_path):
    print("<!DOCTYPE>")
    print("<html>")
    print("<head> <meta charset=\"UTF-8\"> </head>")
    print("<body>")
    print("<h1> Naive File Server </h1>")
    print("<hr/ >")

    # action这里填入当前文件夹
    print("<form action=\"" + file_path_argv + "\" method=\"post\" enctype=\"multipart/form-data\">")
    print("<label for=\"file\">file name</label>")
    print("<input type=\"file\" name=\"file\" id=\"file\" />")
    print("<br />")
    print("<p><input type=\"submit\" value=\"upload\" /></p>")
    print("</form>")
    print("<hr />")

    print("<pre>")

    # 如果当前目录不是"/file",列出上一级目录..
    if dir_path != "/file":
        father_path = os.path.dirname(dir_path)
        print("<a href=\"/?cgi-bin=" + father_path + "\">上一级</a>", end = '')
        print("                                          ", end = '')
        print("目录   " + "-")
    # 列出当前文件夹下所有文件和目录
    for filename in os.listdir(dir_path):
        # 是文件夹，可以跳转
        if os.path.isdir(dir_path + "/" + filename):
            print("<a href=\"/?cgi-bin=" + dir_path + "/" + filename + "\">" + filename + "</a>", end = '')
            print("                                          ", end = '')
            print("目录   " + "-")
        # 是文件，提供下载功能
        else:
            print("<a href=\"" + "/?download=" + dir_path + "/" + filename + "\">" + filename + "</a>", end = '')
            print("                                          ", end = '')
            print("文件   " + "-")

    print("</pre>")
    print("<hr />")
    print("</body>")
    print("</html>")

if __name__ == "__main__":
    # 被popen函数调用时，会传入参数file_path_argv。
    # 它是GET请求中标识所请求文件夹位置的参数,比如说
    # /file/txt,/file/pic,因为config.h中同时把FILE_DIR
    # 设置为了"/file"，也是文件夹在Linux系统中的绝对位置。
    file_path_argv = sys.argv[1]

    # 检查file_path_argv是否合法且存在
    # file_path_argv一定要在FILE_DIR，就是"/file"文件夹下
    # 并且本身是存在的，并且是一个文件夹不是一个文件

    # 检查file_path_argv是否在"/file"文件夹下
    if file_path_argv[0:5] != "/file":
        out_put_error_html(file_path_argv, "This folder is not accessible or does not exist")
    # 检查文件夹是否存在
    elif not(os.path.exists(file_path_argv)):
        out_put_error_html(file_path_argv, "This folder or file does not exist")
    # 检查是否是文件夹
    elif not(os.path.isdir(file_path_argv)):
        out_put_error_html(file_path_argv, "This is not a folder")
    else:
        out_put_files_and_dirs(file_path_argv)