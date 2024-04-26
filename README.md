# TCP-IP-Protocol
C++ 循序渐进地搭建出整个 TCP/IP 协议栈，实现 IP 路由以及 ARP 协议，用自己的协议栈代替 Linux Kernel 的网络协议栈和其他计算机进行通信
## Lab0 
### 实现简单的Web客户端
编写webget，利用操作系统的TCP支持和流套接字抽象在互联网上获取网页的程序，使用HTTP请求格式向指定URL获取响应  
HTTP头部行尾以'\r\n'结尾，Connection: close指示服务器处理完当前请求后直接关闭
```c++
void get_URL(const string &host, const string &path) {
    TCPSocket socket;
    socket.connect(Address(host, "http"));
    socket.write("GET " + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close \r\n" + "\r\n");
    while (!socket.eof())
        cout << socket.read();
    socket.close();
}
```
注意：windows下git clone会把原本Linux项目的换行符改掉
```shell
sed -i -e 's/\r$//' webget_t.sh # 去掉\r
```
### 