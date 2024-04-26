#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

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

void ByteStream::end_input() { ending = true; }

bool ByteStream::input_ended() const { return ending; }

size_t ByteStream::buffer_size() const { return buffer.size(); }

bool ByteStream::buffer_empty() const { return buffer.empty(); }

bool ByteStream::eof() const { return ending && buffer_empty(); }

size_t ByteStream::bytes_written() const { return write_count; }

size_t ByteStream::bytes_read() const { return read_count; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer.size(); }
