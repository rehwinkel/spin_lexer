#pragma once

#include "utf32.hh"

#include <tokens.h>

class lexer {
    utf32::stream stream;
    size_t m_tk_start;
    size_t m_tk_length;

   public:
    lexer(std::istream &stream);
    lexer(utf32::stream stream);
    token next();
    utf32::stringref tk_str();
    size_t tk_start();
    size_t tk_len();
};
