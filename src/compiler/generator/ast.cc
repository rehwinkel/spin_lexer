#include "ast.hh"

ast::ast() : index(ast_counter++) {}

ast::~ast() {}

size_t ast::id() { return this->index; }

std::ostream &ast::print(std::ostream &stream) {
    stream << "EMPTY";
    return stream;
}

ast_set::ast_set(chr_t start, chr_t end, bool negate)
    : ranges{CHAR_RANGE(start, end)}, negate(negate) {}

ast_set::ast_set(std::vector<char_range> ranges, bool negate)
    : ranges(std::move(ranges)), negate(negate) {}

ast_set::~ast_set() {}

std::ostream &ast_set::print(std::ostream &stream) {
    if (this->negate) {
        stream << "Not";
    }
    stream << "Set(" << std::hex;
    for (size_t i = 0; i < this->ranges.size(); i++) {
        chr_t start = this->ranges[i] >> 32;
        chr_t end = this->ranges[i];
        stream << "0x" << start << "-0x" << end;
        if (i + 1 < this->ranges.size()) {
            stream << ", ";
        }
    }
    stream << ")";
    stream << std::dec;
    return stream;
}

ast_cat::ast_cat(std::vector<std::unique_ptr<ast>> children)
    : children(std::move(children)) {}

ast_cat::~ast_cat() {}

std::ostream &ast_cat::print(std::ostream &stream) {
    stream << "Cat(";
    for (size_t i = 0; i < this->children.size(); i++) {
        this->children[i]->print(stream);
        if (i + 1 < this->children.size()) {
            stream << ", ";
        }
    }
    stream << ")";
    return stream;
}

ast_alt::ast_alt(std::vector<std::unique_ptr<ast>> children)
    : children(std::move(children)) {}

ast_alt::~ast_alt() {}

std::ostream &ast_alt::print(std::ostream &stream) {
    stream << "Alt(";
    for (size_t i = 0; i < this->children.size(); i++) {
        this->children[i]->print(stream);
        if (i + 1 < this->children.size()) {
            stream << ", ";
        }
    }
    stream << ")";
    return stream;
}

ast_rep::ast_rep(std::unique_ptr<ast> child, bool accept_empty)
    : child(std::move(child)), accept_empty(accept_empty) {}

ast_rep::~ast_rep() {}

std::ostream &ast_rep::print(std::ostream &stream) {
    stream << "Rep(empty=" << this->accept_empty << ", ";
    child->print(stream);
    stream << ")";
    return stream;
}

void connect_ranges(chr_t begin, chr_t end, bool negate, uint16_t start_state,
                    uint16_t end_state, automaton &machine,
                    std::vector<char_range> &alphabet) {}

autopart ast_set::connect_machine(automaton &machine,
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

    std::set<uint32_t> set_alphabet;
    for (char_range range : this->ranges) {
        chr_t begin = range >> 32;
        chr_t end = range;
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
            set_alphabet.insert(i + 1);
        }
    }
    if (negate) {
        for (size_t i = 1; i < alphabet.size() + 1; i++) {
            if (!set_alphabet.contains(i)) {
                machine.connect(start_state, end_state, i);
            }
        }
    } else {
        for (uint32_t el : set_alphabet) {
            machine.connect(start_state, end_state, el);
        }
    }
    return {start_state, end_state};
}

autopart ast_cat::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  std::map<size_t, std::string> &names,
                                  std::map<uint16_t, std::string> &finals,
                                  uint16_t *state_count) {
    if (this->children.size() > 1) {
        std::vector<autopart> parts;
        for (auto &child : this->children) {
            autopart child_part = child->connect_machine(
                machine, alphabet, names, finals, state_count);
            parts.push_back(child_part);
        }
        for (size_t i = 0; i + 1 < parts.size(); i++) {
            auto &prev = parts[i];
            auto &next = parts[i + 1];
            machine.connect(prev.end, next.start, 0);
        }
        auto a = names.find(this->id());
        if (a != names.end()) {
            finals[parts[parts.size() - 1].end] = a->second;
        }
        return {parts[0].start, parts[parts.size() - 1].end};
    } else {
        return this->children[0]->connect_machine(machine, alphabet, names,
                                                  finals, state_count);
    }
}

autopart ast_alt::connect_machine(automaton &machine,
                                  std::vector<char_range> &alphabet,
                                  std::map<size_t, std::string> &names,
                                  std::map<uint16_t, std::string> &finals,
                                  uint16_t *state_count) {
    uint16_t start_state = *state_count;
    *state_count += 1;
    std::vector<autopart> parts;
    for (auto &child : this->children) {
        autopart child_part = child->connect_machine(machine, alphabet, names,
                                                     finals, state_count);
        parts.push_back(child_part);
    }
    uint16_t end_state = *state_count;
    *state_count += 1;
    auto a = names.find(this->id());
    if (a != names.end()) {
        finals[end_state] = a->second;
    }
    for (autopart &part : parts) {
        machine.connect(start_state, part.start, 0);
        machine.connect(part.end, end_state, 0);
    }
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
    if (this->accept_empty) {
        machine.connect(start_state, end_state, 0);
    }
    machine.connect(start_state, child_part.start, 0);
    machine.connect(child_part.end, end_state, 0);
    machine.connect(child_part.end, child_part.start, 0);
    return {start_state, end_state};
}

void ast_set::construct_alphabet(std::vector<chr_t> &alphabet) {
    for (char_range range : this->ranges) {
        alphabet.push_back(range >> 32);
        alphabet.push_back(range);
    }
}

void ast_alt::construct_alphabet(std::vector<chr_t> &alphabet) {
    for (auto &child : this->children) {
        child->construct_alphabet(alphabet);
    }
}

void ast_cat::construct_alphabet(std::vector<chr_t> &alphabet) {
    for (auto &child : this->children) {
        child->construct_alphabet(alphabet);
    }
}

void ast_rep::construct_alphabet(std::vector<chr_t> &alphabet) {
    this->child->construct_alphabet(alphabet);
}