#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    if ((SYN == false && seg.header().syn == false) || (SYN && seg.header().syn) || (FIN && seg.header().fin))
        return false;  // 收到SYN报文开始接收数据，收到FIN报文停止接收数据
    if (ackno()) {     // 注意确认号和序列号之间可以判断来的报文在不在接收窗口范围内
        uint32_t seq = seg.header().seqno.raw_value(), ack = ackno()->raw_value();
        if (seq + seg.length_in_sequence_space() <= ack || seq >= ack + window_size())
            return false;
    }
    if (seg.header().fin)
        FIN = true;
    if (seg.header().syn) {
        SYN = true;
        isn = seg.header().seqno;
    } else  // 流重组器中已经排好的字节数目可以确定目前字节流的绝对索引
        abs_seq = unwrap(seg.header().seqno, isn, _reassembler.stream_out().bytes_written());
    _reassembler.push_substring(
        seg.payload().copy(), abs_seq - 1, seg.header().fin);  // SYN包可以没有数据却也占一个序列号
    return true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (SYN == false)
        return nullopt;
    return wrap(_reassembler.stream_out().bytes_written(), isn) + SYN + FIN;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
