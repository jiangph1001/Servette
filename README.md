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
make
```

## 启动
```
./servette
```
访问
localhost:8080


