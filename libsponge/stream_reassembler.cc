#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
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
