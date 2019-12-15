# 12.15交报告！！！

12.13之前完成报告，好有修改的时间

# 关于最新的更新

 `servette_event.c`接入了Libevent，已经可以正常编译并启动，但留有大量bug
主要开发依旧以`servette.c`为主
cgi脚本暂不添加到Makefile中,等测试完成后添加  

## Servette

计网实验：使用C语言实现http服务器

目标功能：
- [x] 支持HTTP post 方法，可以上传文件 完成时间:11.19
- [x] 支持HTTP get  方法，可以下载文件 完成时间:11.19
- [ ] 支持配置文件 预计完成时间:12.11
- [x] 支持CGI 显示文件列表 完成时间:11.24
- [x] 支持pthread多线程 完成时间:11.24
- [x] 支持HTTP分块传输 完成时间:11.27
- [x] 支持HTTP持久连接 完成时间:12.4
- [ ] 支持管道 预计完成时间:null
- [ ] 使用openssl库，支持HTTPS 预计完成时间:12.10
- [x] 使用libevent支持多路并发 完成时间:12.9
- [ ] 测试和改bug 预计完成时间: 12.9
- [ ] 实验报告编写  预计完成时间:12.13


### 构建

```
git clone https://github.com/jiangph1001/Servette.git && cd Servette
make
```

### 启动
```
./servette
```
访问
localhost:8080

### 使用

启动后目前默认加载`index.html`  

- html文件读取目录：`www/`
- 文件下载API：`/?download=[文件名] ` 
- CGI功能入口：`/?cgi-bin=[文件目录]`  
- 默认为读取用户目录（即root用户读取`/root`， 普通用户读取`/home/[用户名]`）
- 端口号通过`config.h`修改，修改后需要重新编译（后续通过配置文件读取）

### SSL部分
- 请安装并使用openssl-1.1.1版本编译，我的openssl安装在了 /usr/local/openssl/ 文件夹中
- 请将 源码目录下 cert/cacert.cer 导入浏览器的“受信任的根证书颁发机构”中
- 编译命令为：
- gcc -g servettr_ssl.c -o servette_ssl -pthread -I/usr/local/openssl/include -L/usr/local/openssl/lib -lssl -lcrypto
- 因为我不会写makefile，你可以补上
- 执行命令为./servette_ssl
- 目前所有功能都是正常的，包括上传文件的位置问题，上传文件的权限问题等等。经过检验，上传一个14M的PDF，再下载下来没有问题。
- 去掉了pthread,keep-alive功能



