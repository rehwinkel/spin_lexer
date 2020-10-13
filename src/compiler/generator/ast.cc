#include "ast.hh"

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

ast_cat::ast_cat(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b)
    : child_a(std::move(child_a)), child_b(std::move(child_b)) {}

ast_cat::~ast_cat() {}

std::ostream &ast_cat::print(std::ostream &stream) {
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

autopart ast_cat::connect_machine(automaton &machine,
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

void ast_cat::construct_alphabet(std::vector<chr_t> &alphabet) {
    this->child_a->construct_alphabet(alphabet);
    this->child_b->construct_alphabet(alphabet);
}