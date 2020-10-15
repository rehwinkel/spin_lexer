#include "rules.hh"

#include <utf32.hh>

std::vector<rule> read_rules(std::istream &stream) {
    std::vector<rule> rules;
    std::string line;
    while (std::getline(stream, line)) {
        size_t pos = 0;
        rule r = parse_rule(line, &pos);
        rules.emplace_back(std::move(r));
    }
    return rules;
}

rule parse_rule(std::string &str, size_t *pos) {
    while (!std::isspace(str[*pos])) *pos += 1;
    std::string name = str.substr(0, *pos);
    *pos += 1;
    return rule{name, parse_regex(str, pos)};
}

std::unique_ptr<ast> parse_regex(std::string &str, size_t *pos) {
    std::vector<std::unique_ptr<ast>> alternatives;
    alternatives.emplace_back(parse_sequence(str, pos));
    while (*pos < str.size() && str[*pos] == '|') {
        *pos += 1;
        auto seq = parse_sequence(str, pos);
        alternatives.emplace_back(std::move(seq));
    }
    if (alternatives.size() == 1) {
        return std::move(alternatives[0]);
    }
    return std::make_unique<ast_alt>(std::move(alternatives));
}

std::unique_ptr<ast> parse_sequence(std::string &str, size_t *pos) {
    std::vector<std::unique_ptr<ast>> sequence;
    char next;
    while (*pos < str.size()) {
        next = str[*pos];
        switch (next) {
            case '[':
                *pos += 1;
                sequence.emplace_back(parse_set(str, pos));
                break;
            case '(':
                *pos += 1;
                sequence.emplace_back(parse_regex(str, pos));
                break;
            case ')':
                *pos += 1;
                goto mainloop;
            case '|':
                goto mainloop;
            case '*': {
                *pos += 1;
                auto rep =
                    std::make_unique<ast_rep>(std::move(sequence.back()), true);
                sequence.pop_back();
                sequence.emplace_back(std::move(rep));
                break;
            }
            case '+': {
                *pos += 1;
                auto rep = std::make_unique<ast_rep>(std::move(sequence.back()),
                                                     false);
                sequence.pop_back();
                sequence.emplace_back(std::move(rep));
                break;
            }
            default:
                if (str[*pos] == '\\') {
                    if (str[*pos + 1] == 'L') {
                        *pos += 2;
                        sequence.emplace_back(std::make_unique<ast_set>(
                            std::vector<char_range>(
                                unicode_letters,
                                unicode_letters + sizeof(unicode_letters) /
                                                      sizeof(char_range)),
                            false));
                        break;
                    }
                }
                chr_t ch = read_char(str, pos);
                sequence.emplace_back(
                    std::make_unique<ast_set>(ch, ch + 1, false));
        }
    }
mainloop:
    if (sequence.size() == 1) {
        return std::move(sequence[0]);
    } else {
        return std::make_unique<ast_cat>(std::move(sequence));
    }
}

std::unique_ptr<ast> parse_set(std::string &str, size_t *pos) {
    std::vector<char_range> ranges;
    bool negated = str[*pos] == '^';
    if (negated) {
        *pos += 1;
    }
    char next;
    while (1) {
        next = str[*pos];
        if (next == ']') {
            *pos += 1;
            break;
        }
        ranges.emplace_back(parse_range(str, pos));
    }
    return std::make_unique<ast_set>(std::move(ranges), negated);
}

char_range parse_range(std::string &str, size_t *pos) {
    chr_t start = read_char(str, pos);
    chr_t end = start + 1;
    if (str[*pos] == '-') {
        *pos += 1;
        end = read_char(str, pos) + 1;
    }
    char_range result = CHAR_RANGE(start, end);
    return result;
}

chr_t read_char(std::string &str, size_t *pos) {
    if (str[*pos] == '\\') {
        *pos += 1;
        char next = str[*pos];
        switch (next) {
            case '\\':
                *pos += 1;
                return '\\';
            case 'n':
                *pos += 1;
                return '\n';
            case 'r':
                *pos += 1;
                return '\r';
            case 't':
                *pos += 1;
                return '\t';
            case '+':
                *pos += 1;
                return '+';
            case '*':
                *pos += 1;
                return '*';
            case '[':
                *pos += 1;
                return '[';
            case ']':
                *pos += 1;
                return ']';
            case 'u': {
                *pos += 1;
                chr_t ch = std::stoul(str.substr(*pos, 4), nullptr, 16);
                *pos += 4;
                return ch;
            }
            case 'U': {
                *pos += 1;
                chr_t ch = std::stoul(str.substr(*pos, 8), nullptr, 16);
                *pos += 8;
                return ch;
            }
            default:
                std::string msg("invalid escape: '\\");
                msg.append(std::to_string((unsigned int)next));
                msg.push_back('\'');
                throw std::runtime_error(msg);
        }
    } else {
        char c = str[*pos];
        *pos += 1;
        return c;
    }
}