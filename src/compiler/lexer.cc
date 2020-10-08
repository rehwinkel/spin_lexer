#include "lexer.hh"

lexer::lexer(std::istream &stream) : stream(stream) {}

lexer::lexer(utf32::stream stream) : stream(std::move(stream)) {}

token lexer::next() { return token::IDENTIFIER; }

size_t lexer::tk_start() { return this->m_tk_start; }

size_t lexer::tk_len() { return this->m_tk_length; }

utf32::stringref lexer::tk_str() {
    return utf32::stringref(this->stream.data(), this->m_tk_start,
                            this->m_tk_length);
}