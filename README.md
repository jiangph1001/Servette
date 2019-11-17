# Servette

计网实验：使用C语言实现http服务器

目标功能：
- [ ] 支持HTTP post/get 方法，可以上传或下载文件
- [ ] 支持HTTP分块传输
- [ ] 支持HTTP持久连接和管道
- [ ] 使用openssl库，支持HTTPS
- [ ] 使用libevent支持多路并发


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


