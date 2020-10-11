#include <fstream>
#include <sstream>

#include <utf32.hh>

#include "lexer.hh"

int main(int argc, char const *argv[]) {
    std::string out_dir(argv[1]);
    std::string rules_dir(argv[2]);
    std::cout << "generating lexer in '" << out_dir << "' from rules at '"
              << rules_dir << "'" << std::endl;

    std::ifstream in_rules(rules_dir);
    auto rules = read_rules(in_rules);
    automaton nfa = create_nfa(std::move(rules));

    // TODO:
    std::ofstream out_header(out_dir + "/tokens.h");
    std::ofstream out_code(out_dir + "/lexer.cc");
    out_header << "enum token { IDENTIFIER };";
    out_header.close();
    out_code << "#include <lexer.hh>\n\ntoken lexer::next() { return "
                "token::IDENTIFIER; }";
    out_code.close();
    return 0;
}

automaton create_nfa(std::vector<rule> rules) {
    std::map<size_t, std::string> names;
    for (rule &r : rules) {
        size_t rule_id = reinterpret_cast<size_t>(r.match.get());
        names[rule_id] = r.name;
    }
    std::unique_ptr<ast> match;
    for (rule &r : rules) {
        if (match) {
            match =
                std::make_unique<ast_alt>(std::move(match), std::move(r.match));
        } else {
            match = std::move(r.match);
        }
    }
    std::vector<char_range> alphabet;
    automaton machine(0, std::set<uint16_t>{}, 0, 0);
    uint16_t state_count = 0;
    autopart part = match->connect_machine(machine, alphabet, &state_count);
    machine.set_state_count(state_count);
    machine.set_alphabet_size(alphabet.size());
    automaton dfa = machine.powerset();
    std::cout << dfa << std::endl;
    return machine;
}

std::unique_ptr<ast> parse_regex_rule(std::string &str, size_t *pos);

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
        case 'W':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::NOT_WORD);
        case 's':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::SPACE);
        case 'S':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::NOT_SPACE);
        case 'd':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::DIGIT);
        case 'D':
            *pos += 1;
            return std::make_unique<ast_class>(regex_class::NOT_DIGIT);
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
                    match = std::make_unique<ast_concat>(
                        std::move(match), parse_escape(str, pos));
                } else {
                    match = parse_escape(str, pos);
                }
                break;
            case '*':
                if (match) {
                    match = std::make_unique<ast_concat>(
                        std::move(match), parse_repeat(str, pos));
                } else {
                    match = parse_repeat(str, pos);
                }
                break;
            case '|':
                if (match) {
                    match = std::make_unique<ast_concat>(
                        std::move(match), parse_alternative(str, pos));
                } else {
                    match = parse_alternative(str, pos);
                }
                break;
            case ')':
                return match;
            default:
                if (match) {
                    match = std::make_unique<ast_concat>(
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
                    prev = std::make_unique<ast_concat>(
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

ast::~ast() {}

std::ostream &ast::print(std::ostream &stream) {
    stream << "EMPTY";
    return stream;
}

ast_char::ast_char(chr_t ch) : ch(ch) {}

ast_char::~ast_char() {}

std::ostream &ast_char::print(std::ostream &stream) {
    stream << "Char(0x" << std::hex << this->ch << ")";
    return stream;
}

ast_class::ast_class(regex_class cls) : cls(cls) {}

ast_class::~ast_class() {}

std::ostream &ast_class::print(std::ostream &stream) {
    switch (this->cls) {
        case WORD:
            stream << "WORD";
            break;
        case DIGIT:
            stream << "DIGIT";
            break;
        case SPACE:
            stream << "SPACE";
            break;
        case NOT_WORD:
            stream << "NOT_WORD";
            break;
        case NOT_DIGIT:
            stream << "NOT_DIGIT";
            break;
        case NOT_SPACE:
            stream << "NOT_SPACE";
            break;

        default:
            break;
    }
    return stream;
}

ast_concat::ast_concat(std::unique_ptr<ast> child_a,
                       std::unique_ptr<ast> child_b)
    : child_a(std::move(child_a)), child_b(std::move(child_b)) {}

ast_concat::~ast_concat() {}

std::ostream &ast_concat::print(std::ostream &stream) {
    stream << "Concat(";
    child_a->print(stream);
    stream << ", ";
    child_b->print(stream);
    stream << ")";
    return stream;
}

ast_alt::ast_alt(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b)
    : child_a(std::move(child_a)), child_b(std::move(child_b)) {}

ast_alt::~ast_alt() {}

std::ostream &ast_alt::print(std::ostream &stream) {
    stream << "Alternative(";
    child_a->print(stream);
    stream << ", ";
    child_b->print(stream);
    stream << ")";
    return stream;
}

ast_rep::ast_rep(std::unique_ptr<ast> child) : child(std::move(child)) {}

ast_rep::~ast_rep() {}

std::ostream &ast_rep::print(std::ostream &stream) {
    stream << "Repeat(";
    child->print(stream);
    stream << ")";
    return stream;
}

char_range make_char_range(chr_t start, chr_t end) {
    return ((uint64_t)start << 32) | end;
}

autopart ast::connect_machine(automaton &machine,
                              std::vector<char_range> &alphabet,
                              uint16_t *state_count) {
    throw std::runtime_error("not implemented");
}

autopart ast_char::connect_machine(automaton &machine,
                                   std::vector<char_range> &alphabet,
                                   uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    uint16_t end_state = *state_count;
    *state_count += 1;
    char_range c = make_char_range(this->ch, this->ch + 1);
    auto loc = std::find(alphabet.begin(), alphabet.end(), c);
    uint32_t input;
    if (loc == alphabet.end()) {
        alphabet.push_back(c);
        input = alphabet.size() - 1;
    } else {
        input = loc - alphabet.begin();
    }
    machine.connect(start_state, end_state, input + 1);
    return {start_state, end_state};
}

autopart ast_class::connect_machine(automaton &machine,
                                    std::vector<char_range> &alphabet,
                                    uint16_t *state_count) {
    throw std::runtime_error("not implemented");
}

autopart ast_concat::connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count) {
    throw std::runtime_error("not implemented");
}

autopart ast_alt::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    uint16_t end_state = *state_count;
    *state_count += 1;
    autopart child_a_part =
        this->child_a->connect_machine(machine, alphabet, state_count);
    autopart child_b_part =
        this->child_b->connect_machine(machine, alphabet, state_count);
    machine.connect(start_state, child_a_part.start, 0);
    machine.connect(start_state, child_b_part.start, 0);
    machine.connect(child_a_part.end, end_state, 0);
    machine.connect(child_b_part.end, end_state, 0);
    return {start_state, end_state};
}

autopart ast_rep::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  uint16_t *state_count) {
    throw std::runtime_error("not implemented");
}