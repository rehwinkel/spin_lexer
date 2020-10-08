#pragma once

#include <istream>
#include <string>

namespace utf32 {
    typedef uint32_t chr_t;

    inline chr_t decode_utf8(std::istream &in_stream);
    inline void write_utf8(std::ostream &stream, chr_t ch);

    class string {
        size_t m_length;
        chr_t *m_data;
        void init(std::istream &in_stream);

       public:
        string(std::istream &in_stream);
        string(std::string str);
        string(const string &other);
        string(string &&other);
        ~string();
        size_t len();
        chr_t *data();
        chr_t &operator[](size_t index);
        friend std::ostream &operator<<(std::ostream &stream, string &str);
    };

    class stringref {
        chr_t *m_start;
        size_t m_length;

       public:
        stringref(string &str, size_t offset, size_t length);
        stringref(chr_t *start, size_t length);
        size_t len();
        chr_t *data();
        chr_t &operator[](size_t index);
        friend std::ostream &operator<<(std::ostream &stream, stringref &str);
    };

    class stream {
        string m_data;
        size_t m_position;

       public:
        stream(string s);
        stream(std::istream &in_stream);
        stream(std::string cpp_string);
        stream(const stream &other) = delete;
        stream(stream &&other) = default;
        chr_t get();
        size_t pos();
        bool end();
        string &data();
    };
}