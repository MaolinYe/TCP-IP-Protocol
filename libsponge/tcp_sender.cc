#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

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

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
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

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    timeout += ms_since_last_tick;
    if (!segments.empty() && timeout >= RTO) {  // 存在已发送未确认报文且超时
        _segments_out.emplace(segments.front());
        ++consecutive_retransmit;
        timeout = 0;
        if (rwnd > 0)  // 网络拥堵
            RTO *= 2;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive_retransmit; }

void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}
