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
    auto [dfa, dead, names, final_map, alphabet] =
        create_full_dfa(std::move(rules));

    generate_header(out_dir + "/tokens.h", names);
    generate_cpp(out_dir + "/lexer.cc", dfa, dead, names, final_map, alphabet);
    return 0;
}

void generate_cpp(std::string dir, automaton machine, uint16_t trap,
                  std::map<uint16_t, std::string> names,
                  std::map<uint16_t, uint16_t> final_mapping,
                  std::vector<char_range> alphabet) {
    std::ofstream out_code(dir);
    out_code << "#include <lexer.hh>\n\ntoken lexer::next() {" << std::endl;
    out_code << "    uint16_t state = " << machine.initial << ";" << std::endl
             << "    while (1) {" << std::endl
             << "        switch (state) {" << std::endl
             << "            default:" << std::endl
             << "                return token::ERROR;" << std::endl
             << "        }" << std::endl
             << "    }" << std::endl;
    out_code << "    return token::ERROR;" << std::endl;
    out_code << "}";
    out_code.close();
}

void generate_header(std::string dir, std::map<uint16_t, std::string> names) {
    std::ofstream out_header(dir);
    out_header << "enum token {" << std::endl;
    for (auto &pair : names) {
        out_header << "    " << pair.second << "," << std::endl;
    }
    out_header << "    ERROR," << std::endl;
    out_header << "};";
    out_header.close();
}

std::tuple<automaton, uint16_t, std::map<uint16_t, std::string>,
           std::map<uint16_t, uint16_t>, std::vector<char_range>>
create_full_dfa(std::vector<rule> rules) {
    std::map<size_t, std::string> names;
    for (rule &r : rules) {
        names[r.match->id()] = r.name;
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
    std::vector<chr_t> pre_alphabet;
    match->construct_alphabet(pre_alphabet);
    std::sort(pre_alphabet.begin(), pre_alphabet.end());
    chr_t current = -1;
    for (chr_t p : pre_alphabet) {
        if (current == -1) {
            current = p;
        } else {
            if (current != p) {
                alphabet.push_back(make_char_range(current, p));
                current = p;
            }
        }
    }

    std::map<uint16_t, std::string> finals;
    automaton machine(0, std::set<uint16_t>{}, 0, 0);
    uint16_t state_count = 0;
    autopart part =
        match->connect_machine(machine, alphabet, names, finals, &state_count);
    machine.states = state_count;
    machine.alphabet = alphabet.size();
    std::set<uint16_t> actual_finals;
    for (auto &fin : finals) {
        actual_finals.insert(fin.first);
    }
    machine.finals = actual_finals;
    std::map<uint16_t, uint16_t> final_mapping;
    auto [dfa, dead] = machine.powerset(final_mapping);
    return std::make_tuple(dfa, dead, finals, final_mapping, alphabet);
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

ast::ast() : index(ast_counter++) {}

ast::~ast() {}

size_t ast::id() { return this->index; }

std::ostream &ast::print(std::ostream &stream) {
    stream << "EMPTY";
    return stream;
}

ast_char::ast_char(chr_t ch) : ch(ch) {}

ast_char::~ast_char() {}

std::ostream &ast_char::print(std::ostream &stream) {
    stream << "Char(0x" << std::hex << this->ch << std::dec << ")";
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

void connect_range(chr_t begin, chr_t end, uint16_t start_state,
                   uint16_t end_state, automaton &machine,
                   std::vector<char_range> &alphabet) {
    size_t start_index = std::find_if(alphabet.begin(), alphabet.end(),
                                      [&begin](const char_range &arg) {
                                          return begin == arg >> 32;
                                      }) -
                         alphabet.begin();
    size_t end_index = std::find_if(alphabet.begin(), alphabet.end(),
                                    [&end](const char_range &arg) {
                                        return end == (chr_t)arg;
                                    }) -
                       alphabet.begin();
    for (size_t i = start_index; i < end_index + 1; i++) {
        machine.connect(start_state, end_state, i + 1);
    }
}

void connect_char(chr_t ch, uint16_t start_state, uint16_t end_state,
                  automaton &machine, std::vector<char_range> &alphabet) {
    connect_range(ch, ch + 1, start_state, end_state, machine, alphabet);
}

autopart ast_char::connect_machine(automaton &machine,
                                   std::vector<char_range> &alphabet,
                                   std::map<size_t, std::string> &names,
                                   std::map<uint16_t, std::string> &finals,
                                   uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    uint16_t end_state = *state_count;
    *state_count += 1;
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[end_state] = a->second;
    }
    connect_char(this->ch, start_state, end_state, machine, alphabet);
    return {start_state, end_state};
}

autopart ast_class::connect_machine(automaton &machine,
                                    std::vector<char_range> &alphabet,
                                    std::map<size_t, std::string> &names,
                                    std::map<uint16_t, std::string> &finals,
                                    uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    uint16_t end_state = *state_count;
    *state_count += 1;
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[end_state] = a->second;
    }

    switch (this->cls) {
        case WORD:
            connect_range('a', 'z' + 1, start_state, end_state, machine,
                          alphabet);
            connect_range('A', 'Z' + 1, start_state, end_state, machine,
                          alphabet);
            break;
        case DIGIT:
            connect_range('0', '9' + 1, start_state, end_state, machine,
                          alphabet);
            break;
        case SPACE:
            connect_char(' ', start_state, end_state, machine, alphabet);
            connect_char('\t', start_state, end_state, machine, alphabet);
            connect_char('\n', start_state, end_state, machine, alphabet);
            connect_char('\f', start_state, end_state, machine, alphabet);
            connect_char('\r', start_state, end_state, machine, alphabet);
            break;
    }
    return {start_state, end_state};
}

autopart ast_concat::connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count) {
    autopart child_a_part = this->child_a->connect_machine(
        machine, alphabet, names, finals, state_count);
    autopart child_b_part = this->child_b->connect_machine(
        machine, alphabet, names, finals, state_count);
    machine.connect(child_a_part.end, child_b_part.start, 0);
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[child_b_part.end] = a->second;
    }
    return {child_a_part.start, child_b_part.end};
}

autopart ast_alt::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  std::map<size_t, std::string> &names,
                                  std::map<uint16_t, std::string> &finals,
                                  uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    autopart child_a_part = this->child_a->connect_machine(
        machine, alphabet, names, finals, state_count);
    autopart child_b_part = this->child_b->connect_machine(
        machine, alphabet, names, finals, state_count);
    uint16_t end_state = *state_count;
    *state_count += 1;
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[end_state] = a->second;
    }
    machine.connect(start_state, child_a_part.start, 0);
    machine.connect(start_state, child_b_part.start, 0);
    machine.connect(child_a_part.end, end_state, 0);
    machine.connect(child_b_part.end, end_state, 0);
    return {start_state, end_state};
}

autopart ast_rep::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  std::map<size_t, std::string> &names,
                                  std::map<uint16_t, std::string> &finals,
                                  uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    autopart child_part = this->child->connect_machine(machine, alphabet, names,
                                                       finals, state_count);
    uint16_t end_state = *state_count;
    *state_count += 1;
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[end_state] = a->second;
    }
    machine.connect(start_state, end_state, 0);
    machine.connect(start_state, child_part.start, 0);
    machine.connect(child_part.end, end_state, 0);
    machine.connect(child_part.end, child_part.start, 0);
    return {start_state, end_state};
}

void ast_char::construct_alphabet(std::vector<chr_t> &alphabet) {
    alphabet.push_back(this->ch);
    alphabet.push_back(this->ch + 1);
}

void ast_class::construct_alphabet(std::vector<chr_t> &alphabet) {
    switch (this->cls) {
        case WORD:
            alphabet.push_back('a');
            alphabet.push_back('z' + 1);
            alphabet.push_back('A');
            alphabet.push_back('Z' + 1);
            break;
        case DIGIT:
            alphabet.push_back('0');
            alphabet.push_back('9' + 1);
            break;
        case SPACE:
            alphabet.push_back(' ');
            alphabet.push_back(' ' + 1);
            alphabet.push_back('\t');
            alphabet.push_back('\t' + 1);
            alphabet.push_back('\n');
            alphabet.push_back('\n' + 1);
            alphabet.push_back('\f');
            alphabet.push_back('\f' + 1);
            alphabet.push_back('\r');
            alphabet.push_back('\r' + 1);
            break;
    }
}

void ast_alt::construct_alphabet(std::vector<chr_t> &alphabet) {
    this->child_a->construct_alphabet(alphabet);
    this->child_b->construct_alphabet(alphabet);
}

void ast_rep::construct_alphabet(std::vector<chr_t> &alphabet) {
    this->child->construct_alphabet(alphabet);
}

void ast_concat::construct_alphabet(std::vector<chr_t> &alphabet) {
    this->child_a->construct_alphabet(alphabet);
    this->child_b->construct_alphabet(alphabet);
}