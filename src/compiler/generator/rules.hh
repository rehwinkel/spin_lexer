#pragma once

#include "ast.hh"

struct rule {
    std::string name;
    std::unique_ptr<ast> match;
};

std::vector<rule> read_rules(std::istream &stream);
rule parse_rule(std::string &str, size_t *pos);
std::unique_ptr<ast> parse_regex(std::string &str, size_t *pos);
std::unique_ptr<ast> parse_sequence(std::string &str, size_t *pos);
std::unique_ptr<ast> parse_set(std::string &str, size_t *pos);
char_range parse_range(std::string &str, size_t *pos);
chr_t read_char(std::string &str, size_t *pos);