# TCP-IP-Protocol
斯坦福计算机网络课程项目，使用C++循序渐进地搭建出整个TCP/IP协议栈，实现IP路由以及ARP协议，最后用自己实现的协议栈代替Linux Kernel的网络协议栈和其他计算机进行通信
#### Lab0 实现简单的Web客户端和有序字节流类（内存管道）  2h(Clion+wsl) 2h 2h
#### Lab1 实现流重组器，将交叉重叠乱序的字节流重新组装成正确有序的字节流 6h
#### Lab2 实现TCP接收器，32位序列号到64位字节流索引的转换，计算确认号，告知发送方接收窗口的大小来实现流量控制 10h
#### Lab3 实现TCP发送器，读取应用程序创建的字节流转换成TCP报文，处理接收方的确认号和接收窗口，实现超时重传 5h
## Lab0 字节流
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
## Lab1 字节流重组器
#### Lab1 实现流重组器，将交叉重叠乱序的字节流重新组装成正确有序的字节流
最简单粗暴的思想，用一个哈希表存储数据索引对应的字节，这样可以解决交叉重叠的问题，乱序可以从0开始查找写入流，哈希查找很快，但是插入删除很慢，实测1.24sec，同样改用红黑树映射，查找慢，但是插入删除快，实测1.83sec  
重组器装的是字节流类ByteStream里面已经重组好的字节流和等待重组的字节流  
无效数据：字节流尾部索引小于已重组数据索引，或者字节流首部索引大于已读数据索引+capacity
![streamReassembler.png](images/streamReassembler.png)
```c++
    size_t assembledIndex = 0;
    std::unordered_map<size_t, char> buffer{};
    bool ending = false;
```
```c++
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (_output.buffer_size() + buffer.size() == _capacity || index + data.size() < assembledIndex ||
        assembledIndex + _capacity - _output.buffer_size() <= index)
        return;
    size_t end = min(index + data.size(), assembledIndex + _capacity - _output.buffer_size());
    size_t start = max(assembledIndex, index);
    if (end == index + data.size() && eof)  // 如果能够完整写入末尾并且eof说明已经装上了eof
        ending = true;
    for (size_t i = start; i < end; ++i)
        buffer[i] = data[i - index];  // 数据本身索引需要和整体数据索引对应
    string segement;
    auto &&byte = buffer.find(assembledIndex);
    while (byte != buffer.end()) {
        segement += buffer[assembledIndex];
        buffer.erase(byte);
        byte = buffer.find(++assembledIndex);
    }
    if (!segement.empty())
        _output.write(segement);
    if (buffer.empty() && ending)  // 如果重组完所有片段并且收到了结尾片段
        _output.end_input();
}

size_t StreamReassembler::unassembled_bytes() const { return buffer.size(); }

bool StreamReassembler::empty() const { return buffer.empty(); }
```
优化思路：用索引映射字符需要大量的查找、插入和删除操作，可以存储每段字符串和其索引，这样以字符串为整体可以减少大量的字符插入和删除，交叉重叠的就进行合并，合并需要遍历存储的所有字符串区间判断是否存在交叉，考虑使用链表，而且本身链表插入删除就方便，并且如果按索引进行排序插入，查找可以使用二分
## Lab2 TCP接收器
#### 实现TCP接收器，32位序列号到64位字节流索引的转换，计算确认号，告知发送方接收窗口的大小来实现流量控制
将64位的流绝对索引转变成TCP报文中32位的序列号，为了安全序列号从一个随机号开始，因此流索引需要加上这个随机号再转换类型
```c++
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return WrappingInt32(static_cast<uint32_t>(n) + isn.raw_value()); }
```
同时需要将32位的序列号转换成64位的流绝对索引，这里提供的一个最近收到报文的64位流绝对索引checkpoint，可以用来确定当前报文的绝对位置，因为一般来说，两个报文的位置不会超过TCP序列号的范围，即2^32，所以可以先用上面的wrap计算出checkpoint的序列号，要求返回和checkpoint最近的一个索引，先算他们两个的序列号之差，肯定落在2^32的范围里面，如果差大于2^31，说明最近的索引不在同一个2^32里面
```c++
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint32_t now = n.raw_value(), last = wrap(checkpoint, isn).raw_value();
    if (now < last) {
        uint32_t distance = last - now;
        if (distance > 1ul << 32 / 2 || checkpoint == 0)
            return checkpoint + (1ul << 32) - distance;
        return checkpoint - distance;
    }
    uint32_t distance = now - last;
    if (distance > 1ul << 32 / 2 && checkpoint)
        return checkpoint + distance - (1ul << 32);
    return checkpoint + distance;
}
```
注意：有符号短数和无符号长数运算时会先进行位扩展，即短变长，再进行符号的转变，即有符号变无符号  
要实现TCP接收器，将TCP报文中的序列号转换成流重组器可用的字节流索引，计算接收窗口大小，收到SYN报文开始接收数据，收到FIN报文停止接收数据  
通过流重组器中已经排好的字节数目可以确定目前字节流的绝对索引，但是流重组器的索引从0开始，但是SYN包可以没有数据却也占一个序列号，所以整体数据往前移动一位  
还需要注意确认号和序列号之间可以判断来的报文在不在接收窗口范围内
```c++
    size_t _capacity;
    WrappingInt32 isn;
    uint64_t abs_seq = 1;
    bool SYN = false;
    bool FIN = false;
```
```c++
bool TCPReceiver::segment_received(const TCPSegment &seg) {
    if ((SYN == false && seg.header().syn == false) || (SYN && seg.header().syn) || (FIN && seg.header().fin))
        return false; // 收到SYN报文开始接收数据，收到FIN报文停止接收数据 
    if (ackno()) { // 注意确认号和序列号之间可以判断来的报文在不在接收窗口范围内
        uint32_t seq = seg.header().seqno.raw_value(), ack = ackno()->raw_value();
        if (seq + seg.length_in_sequence_space() <= ack || seq >= ack + window_size())
            return false;
    }
    if (seg.header().fin)
        FIN = true;
    if (seg.header().syn) {
        SYN = true;
        isn = seg.header().seqno;
    } else // 流重组器中已经排好的字节数目可以确定目前字节流的绝对索引
        abs_seq = unwrap(seg.header().seqno, isn, _reassembler.stream_out().bytes_written());
    _reassembler.push_substring(seg.payload().copy(), abs_seq - 1, seg.header().fin); // SYN包可以没有数据却也占一个序列号
    return true;
}
```
确认号为期待收到的下一个序列号，即是当前已接收的字节流长度加上初始序列号，因为SYN和FIN都要占一个字节，所以要加上
```c++
optional<WrappingInt32> TCPReceiver::ackno() const {
    if (SYN == false)
        return nullopt;
    return wrap(_reassembler.stream_out().bytes_written(), isn) + SYN + FIN;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
```
## Lab3 TCP发送器
#### 实现TCP发送器，读取应用程序创建的字节流转换成TCP报文，处理接收方的确认号和接收窗口，实现超时重传  
使用队列记录已发送未确认的报文
```c++
    std::queue<TCPSegment> segments{};     // 可能需要重传的报文队列
    size_t tbc = 0;                        // to be confirmed 已经发送待确认的字节长度
    size_t consecutive_retransmit = 0;     // 连续重传数目
    uint16_t rwnd = 1;                     // 接收窗口
    size_t RTO = TCPConfig::TIMEOUT_DFLT;  // 超时重传等待时间
    size_t timeout = 0;                    // RTO计时
    bool SYN = false;
    bool FIN = false;
```
由Lab2的实现可以知道，接收窗口的大小是流重组器的容量减去已经排好序的字节流长度，也就是包括了已经发送但未确认的数据，因此需要在接收窗口大于已经发送但未确认的字节数目才能继续发送数据  
首发设置SYN，注意SYN占一个字节，因此首发的时候能够发送的数据长度减一，同样FIN占一个字节，需要在接收窗口仍有空余时发送  
如果还没有在超时追踪的报文，那么重置超时定时器，如果是空包或者FIN包，那么后面不用发了
```c++
uint64_t TCPSender::bytes_in_flight() const { return tbc; }

void TCPSender::fill_window() {
    while (rwnd > tbc) {
        TCPSegment segment;
        if (SYN == false)  // 首发
            segment.header().syn = SYN = true;
        segment.header().seqno = next_seqno();
        size_t payload_size =
            min(TCPConfig::MAX_PAYLOAD_SIZE, rwnd - tbc - segment.header().syn);  // SYN包SYN占一个字节
        string payload(move(_stream.read(payload_size)));
        if (_stream.eof() && FIN == false && payload.size() + tbc < rwnd)  // FIN包FIN占一个字节
            segment.header().fin = FIN = true;
        segment.payload() = Buffer(move(payload));
        if (segment.length_in_sequence_space() == 0)  // 空包
            break;
        _segments_out.emplace(segment);  // 放到发送窗口
        if (segments.empty()) {          // 超时定时
            timeout = 0;
            RTO = _initial_retransmission_timeout;
        }
        tbc += segment.length_in_sequence_space();
        segments.emplace(segment);  // 超时追踪
        _next_seqno += segment.length_in_sequence_space();
        if (FIN)  // FIN包
            break;
    }
}
```
通过接收方返回的确认号得到流的绝对索引，如果大于当前的索引无效，然后查看已发送未确认报文中是否有被确认的报文  
因为是按索引顺序入队的，如果前面的未确认后面的肯定没确认，因为接收方是累加确认方式  
注意如果接收窗口为0不能不发送报文，因为需要知道接收窗口什么时候不为0就必须发送报文，所以要改成1
```c++
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);
    if (_next_seqno < abs_ackno)  // 确认号大于序列号无效
        return false;
    while (!segments.empty()) {
        auto &&segment = segments.front();
        if (abs_ackno >= unwrap(segment.header().seqno, _isn, _next_seqno) + segment.length_in_sequence_space()) {
            tbc -= segment.length_in_sequence_space();
            segments.pop();
            timeout = 0;
            RTO = _initial_retransmission_timeout;
            consecutive_retransmit = 0;
        } else
            break;
    }
    rwnd = window_size ? window_size : 1;  // 零窗口探测
    fill_window();
    return true;
}
```
实现超时重传，因为接收方是累加确认的方式，返回的确认号是它所期待收到的第一个序列号，因此只需要重传第一个未接收到的报文，如果接收窗口大于0但是依然超时说明网络拥堵，重传等待时间翻倍
```c++
void TCPSender::tick(const size_t ms_since_last_tick) {
    timeout += ms_since_last_tick;
    if (!segments.empty() && timeout >= RTO) {  // 存在已发送未确认报文且超时
        _segments_out.emplace(segments.front());
        ++consecutive_retransmit;
        timeout = 0;
        if (rwnd > 0) // 网络拥堵
            RTO *= 2;
    }
}
```
设置一个空TCP报文，序列号为当前序列号，放进发送窗口
```c++
unsigned int TCPSender::consecutive_retransmissions() const { return consecutive_retransmit; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}

```