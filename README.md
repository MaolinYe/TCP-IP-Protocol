# TCP-IP-Protocol
斯坦福计算机网络课程项目，使用C++循序渐进地搭建出整个TCP/IP协议栈，实现IP路由以及ARP协议，最后用自己实现的协议栈代替Linux Kernel的网络协议栈和其他计算机进行通信
#### Lab0 实现简单的Web客户端和有序字节流类（内存管道）  2h(Clion+wsl) 2h 2h
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
### 实现内存管道：有序字节流类
实现一个有序字节流类，可以往流中按字节写入数据在末尾，可以在流首部按字节读取数据  
可以使用队列实现流，用字节队列模拟字节流，有容量限制
```c++
    std::deque<char> buffer{};
    size_t _capacity = 0;
    size_t read_count = 0;
    size_t write_count = 0;
    bool ending = false;
```
如果写入数据大于流容量，则能够写多少就写多少
```shell
ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

size_t ByteStream::write(const string &data) {
    size_t data_size = data.size();
    if (data_size + buffer.size() > _capacity)
        data_size = _capacity - buffer.size();
    for (size_t i = 0; i < data_size; ++i)
        buffer.push_back(data[i]);
    write_count += data_size;
    return data_size;
}
```
读取的数据长度不能超过流的长度
```c++
//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    long read = min(len, buffer.size());
    return string().assign(buffer.begin(), buffer.begin() + read);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t out = min(len, buffer.size());
    read_count += out;
    while (out--)
        buffer.pop_front();
}
```
其他
```c++
void ByteStream::end_input() { ending = true; }

bool ByteStream::input_ended() const { return ending; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.empty(); }

bool ByteStream::eof() const { return ending && buffer_empty(); }

size_t ByteStream::bytes_written() const { return write_count; }

size_t ByteStream::bytes_read() const { return read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer.size(); }
```