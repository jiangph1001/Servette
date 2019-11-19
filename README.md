# Servette

计网实验：使用C语言实现http服务器

目标功能：
- [x] 支持HTTP post 方法，可以上传文件 完成时间:11.19
- [x] 支持HTTP get  方法，可以下载文件 完成时间:11.19
- [ ] 支持配置文件 预计完成时间:12.31
- [ ] 支持CGI 显示文件列表 预计完成时间:11.24
- [ ] *支持日志输出 预计完成时间:12.8
- [ ] 支持pthread多线程 预计完成时间:11.24
- [x] 支持HTTP分块传输 预计完成时间:12.1
- [ ] 支持HTTP持久连接和管道 预计完成时间:12.1
- [ ] 使用openssl库，支持HTTPS 预计完成时间:12.1
- [ ] 使用libevent支持多路并发 预计完成时间:12.8


### 构建

```
git clone https://github.com/jiangph1001/Servette.git && cd Servette
make && make install
```
目前可以实现读取 /data/www/目录下的html文件  
该目录暂时可以通过config.h修改，但是需要同步修改make install

## 启动
```
./servette
```
访问
localhost:8080

启动后目前默认加载index.html  
/?file=[文件名]实现文件下载，但目前存在问题  
端口号通过config.h修改，修改后需要重新编译


