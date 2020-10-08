#include <fstream>
#include <sstream>

#include "lexer.hh"

int main(int argc, char const *argv[]) {
    std::string out_dir(argv[1]);
    std::string rules_dir(argv[2]);
    std::cout << "generating lexer in '" << out_dir << "' from rules at '"
              << rules_dir << "'" << std::endl;

    std::ifstream in_rules(rules_dir);
    auto rules = read_rules(in_rules);

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
            std::vector<std::unique_ptr<ast>> seq;
            for (char &ch : rule_str) {
                seq.emplace_back(std::make_unique<ast_char>(ch));
            }
            auto match = std::make_unique<ast_seq>(std::move(seq));
            std::cout << name << ": ";
            match->print(std::cout);
            std::cout << std::endl;

            rules.emplace_back(rule{name, std::move(match)});
        } else if (line[0] == 'R') {
            // throw std::runtime_error("unimplemented");
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

ast_seq::ast_seq(std::vector<std::unique_ptr<ast>> children)
    : children(std::move(children)) {}

ast_seq::~ast_seq() {}

std::ostream &ast_seq::print(std::ostream &stream) {
    stream << "Seq(";
    for (size_t i = 0; i < children.size(); i++) {
        auto &child = children.at(i);
        child->print(stream);
        if (i + 1 < children.size()) {
            stream << ", ";
        }
    }
    stream << ")";
    return stream;
}