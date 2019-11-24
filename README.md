**12.15交报告！！**

# Servette

计网实验：使用C语言实现http服务器

目标功能：
- [x] 支持HTTP post 方法，可以上传文件 完成时间:11.19
- [x] 支持HTTP get  方法，可以下载文件 完成时间:11.19
- [ ] 支持配置文件 预计完成时间:12.3
- [x] 支持CGI 显示文件列表 完成时间:11.24
- [ ] *支持日志输出 预计完成时间:12.8
- [x] 支持pthread多线程 完成时间:11.24
- [ ] 支持HTTP分块传输 预计完成时间:11.27
- [ ] 支持HTTP持久连接和管道 预计完成时间:11.28
- [ ] 使用openssl库，支持HTTPS 预计完成时间:12.1
- [ ] 使用libevent支持多路并发 预计完成时间:12.8


### 构建

```
git clone https://github.com/jiangph1001/Servette.git && cd Servette
make
```

## 启动
```
./servette
```
访问
localhost:8080

## 使用

启动后目前默认加载`index.html`  


- html文件读取目录：`www/`
- 文件下载API：`/?download=[文件名] ` 
- CGI功能入口：`/#cgi-bin=[文件目录]`  
- 默认为读取用户目录（即root用户读取`/root`， 普通用户读取`/home/[用户名]`）(暂未实现，后续版本修改)  
- 端口号通过`config.h`修改，修改后需要重新编译（后续通过配置文件读取）



