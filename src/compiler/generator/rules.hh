#pragma once

#include "ast.hh"

struct rule {
    std::string name;
    std::unique_ptr<ast> match;
};

std::vector<rule> read_rules(std::istream &stream);
std::unique_ptr<ast> parse_regex_rule(std::string &str, size_t *pos);