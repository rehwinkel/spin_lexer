#include "utf32.hh"

using namespace utf32;

#include <sstream>
#include <cstring>

struct TypeUTF8 {
    uint8_t mask;
    uint8_t value;
    uint8_t extra;
};

static const TypeUTF8 utf8_types[] = {
    {0x80, 0x00, 0}, {0xe0, 0xc0, 1}, {0xf0, 0xe0, 2}, {0xf8, 0xf0, 3}};

inline chr_t finish_read_utf8(std::istream &stream, chr_t result,
                              uint8_t count) {
    for (; count; --count) {
        chr_t next = stream.get();
        if (next == EOF) {
            throw std::runtime_error("unexpected end of utf8 sequence");
        }
        if ((next & 0xC0) != 0x80) {
            throw std::runtime_error("invalid utf8 continuation character");
        }
        result = (result << 6) | (next & 0x3f);
    }
    return result;
}

chr_t utf32::decode_utf8(std::istream &stream) {
    chr_t next = stream.get();
    if (next == EOF) {
        return EOF;
    }
    for (auto const &utf8_type : utf8_types) {
        if ((next & utf8_type.mask) == utf8_type.value) {
            return finish_read_utf8(stream, next & ~utf8_type.mask,
                                    utf8_type.extra);
        }
    }
    throw std::runtime_error("invalid utf8 start character");
}

void utf32::write_utf8(std::ostream &stream, chr_t ch) {
    if (ch < 0x80) {
        stream << (char)ch;
    } else if (ch < 0x800) {
        stream << (char)(((ch >> 6) & 0x1f) | 0xc0)
               << (char)((ch & 0x3f) | 0x80);
    } else if (ch < 0x10000) {
        stream << (char)(((ch >> 12) & 0xf) | 0xe0)
               << (char)(((ch >> 6) & 0x3f) | 0x80)
               << (char)((ch & 0x3f) | 0x80);
    } else {
        stream << (char)(((ch >> 18) & 0x7) | 0xf0)
               << (char)(((ch >> 12) & 0x3f) | 0x80)
               << (char)(((ch >> 6) & 0x3f) | 0x80)
               << (char)((ch & 0x3f) | 0x80);
    }
}

string::string(std::istream &in_stream) { this->init(in_stream); }

string::string(std::string str) {
    std::istringstream in_stream(str);
    this->init(in_stream);
}

string::string(string &&other) {
    this->m_data = other.m_data;
    other.m_data = nullptr;
    this->m_length = other.m_length;
    other.m_length = 0;
}

string::string(const string &other) {
    this->m_length = other.m_length;
    this->m_data = (chr_t *)std::malloc(sizeof(chr_t) * this->m_length);
    std::memcpy(this->m_data, other.m_data, sizeof(chr_t) * this->m_length);
}

void string::init(std::istream &in_stream) {
    this->m_length = 0;
    size_t cap = 4;
    this->m_data = (chr_t *)std::malloc(sizeof(chr_t) * cap);
    while (1) {
        chr_t a = utf32::decode_utf8(in_stream);
        if (a == -1) {
            break;
        }
        if (this->m_length + 1 == cap) {
            cap <<= 1;
            chr_t *tmp_data =
                (chr_t *)std::realloc(this->m_data, sizeof(chr_t) * cap);
            if (tmp_data != nullptr) {
                this->m_data = tmp_data;
            } else {
                throw std::runtime_error("out of memory while reading utf8");
            }
        }
        this->m_data[this->m_length] = a;
        this->m_length++;
    }
    this->m_data =
        (chr_t *)std::realloc(this->m_data, sizeof(chr_t) * this->m_length);
}

string::~string() { std::free(this->m_data); }

size_t string::len() { return this->m_length; }

chr_t *string::data() { return this->m_data; }

chr_t &string::operator[](size_t index) { return this->m_data[index]; }

stringref::stringref(string &str, size_t offset, size_t length) {
    this->m_start = str.data() + offset;
    this->m_length = length;
}

stringref::stringref(chr_t *start, size_t length) {
    this->m_start = start;
    this->m_length = length;
}

size_t stringref::len() { return this->m_length; }

chr_t *stringref::data() { return this->m_start; }

chr_t &stringref::operator[](size_t index) { return this->m_start[index]; }

namespace utf32 {
    std::ostream &operator<<(std::ostream &stream, string &str) {
        for (size_t i = 0; i < str.len(); i++) {
            write_utf8(stream, str[i]);
        }
        return stream;
    }

    std::ostream &operator<<(std::ostream &stream, stringref &str) {
        for (size_t i = 0; i < str.len(); i++) {
            write_utf8(stream, str[i]);
        }
        return stream;
    }
}

stream::stream(std::istream &in_stream) : m_data(in_stream), m_position(0) {}

stream::stream(std::string in_string) : m_data(in_string), m_position(0) {}

stream::stream(string in_utf32_string)
    : m_data(in_utf32_string), m_position(0) {}

chr_t stream::get() { return this->m_data[this->m_position++]; }

size_t stream::pos() { return this->m_position; }

bool stream::end() { return this->m_position == this->m_data.len(); }

string &stream::data() { return this->m_data; }