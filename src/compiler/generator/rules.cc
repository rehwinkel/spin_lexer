#include "rules.hh"

#include <utf32.hh>

std::vector<rule> read_rules(std::istream &stream) {
    std::vector<rule> rules;
    std::string line;
    while (std::getline(stream, line)) {
        size_t space_pos = line.find(' ');
        std::string name = line.substr(2, space_pos - 2);
        std::string rule_str = line.substr(space_pos + 1);
        if (line[1] != ':') {
            throw std::runtime_error("failed to parse rules at line: '" + line +
                                     "'");
        }
        if (line[0] == 'L') {
            std::unique_ptr<ast> prev;
            utf32::string utfstring(rule_str);
            for (size_t i = 0; i < utfstring.len(); i++) {
                chr_t ch = utfstring[i];
                if (prev) {
                    prev = std::make_unique<ast_cat>(
                        std::move(prev), std::make_unique<ast_char>(ch));
                } else {
                    prev = std::make_unique<ast_char>(ch);
                }
            }
            std::cout << name << ": ";
            prev->print(std::cout);
            std::cout << std::endl;
            rules.emplace_back(rule{name, std::move(prev)});
        } else if (line[0] == 'R') {
            size_t pos = 0;
            auto match = parse_regex_rule(rule_str, &pos);
            std::cout << name << ": ";
            match->print(std::cout);
            std::cout << std::endl;
            rules.emplace_back(rule{name, std::move(match)});
        } else {
            throw std::runtime_error(
                "invalid rule type (must be either 'L' or 'R'): '" +
                std::to_string(line[0]) + "'");
        }
    }
    return rules;
}

std::unique_ptr<ast> parse_repeat(std::string &str, size_t *pos) {
    *pos += 1;
    if (str[*pos] != '(') {
        throw std::runtime_error("expected '(' while parsing regex: " + str);
    }
    *pos += 1;
    auto match = std::make_unique<ast_rep>(parse_regex_rule(str, pos));
    if (str[*pos] != ')') {
        throw std::runtime_error("expected ')' while parsing regex: " + str);
    }
    *pos += 1;
    return match;
}

std::unique_ptr<ast> parse_alternative(std::string &str, size_t *pos) {
    *pos += 1;
    if (str[*pos] != '(') {
        throw std::runtime_error("expected '(' while parsing regex: " + str);
    }
    *pos += 1;
    auto match = parse_regex_rule(str, pos);
    if (str[*pos] != ')') {
        throw std::runtime_error("expected ')' while parsing regex: " + str);
    }
    *pos += 1;
    if (str[*pos] != '(') {
        throw std::runtime_error("expected '(' while parsing regex: " + str);
    }
    *pos += 1;
    auto match2 = parse_regex_rule(str, pos);
    if (str[*pos] != ')') {
        throw std::runtime_error("expected ')' while parsing regex: " + str);
    }
    *pos += 1;
    return std::make_unique<ast_alt>(std::move(match), std::move(match2));
}

std::unique_ptr<ast> parse_escape(std::string &str, size_t *pos) {
    *pos += 1;
    switch (str[*pos]) {
        case 'w':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::WORD);
        case 's':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::SPACE);
        case 'd':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::DIGIT);
        case '*':
            *pos += 1;
            return std::make_unique<ast_char>('*');
        case '|':
            *pos += 1;
            return std::make_unique<ast_char>('|');
        case '(':
            *pos += 1;
            return std::make_unique<ast_char>('(');
        case ')':
            *pos += 1;
            return std::make_unique<ast_char>(')');
        case 'U': {
            *pos += 1;
            chr_t code = std::stoi(str.substr(*pos, 6), 0, 16);
            *pos += 6;
            return std::make_unique<ast_char>(code);
        }
        case 'u': {
            *pos += 1;
            chr_t code = std::stoi(str.substr(*pos, 4), 0, 16);
            *pos += 4;
            return std::make_unique<ast_char>(code);
        }
        case 'n': {
            *pos += 1;
            return std::make_unique<ast_char>('\n');
        }
        default:
            throw std::runtime_error("error while parsing regex: " + str);
    }
}

std::unique_ptr<ast> parse_regex_rule(std::string &str, size_t *pos) {
    std::unique_ptr<ast> match;
    while (*pos < str.size()) {
        switch (str[*pos]) {
            case '\\':
                if (match) {
                    match = std::make_unique<ast_cat>(std::move(match),
                                                      parse_escape(str, pos));
                } else {
                    match = parse_escape(str, pos);
                }
                break;
            case '*':
                if (match) {
                    match = std::make_unique<ast_cat>(std::move(match),
                                                      parse_repeat(str, pos));
                } else {
                    match = parse_repeat(str, pos);
                }
                break;
            case '|':
                if (match) {
                    match = std::make_unique<ast_cat>(
                        std::move(match), parse_alternative(str, pos));
                } else {
                    match = parse_alternative(str, pos);
                }
                break;
            case ')':
                return match;
            default:
                if (match) {
                    match = std::make_unique<ast_cat>(
                        std::move(match),
                        std::make_unique<ast_char>(str[*pos]));
                } else {
                    match = std::make_unique<ast_char>(str[*pos]);
                }
                *pos += 1;
                break;
        }
    }
    return match;
}