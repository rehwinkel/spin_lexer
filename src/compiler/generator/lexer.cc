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
    for (const rule &r : rules) {
        std::cout << r.name << ": ";
        r.match->print(std::cout) << std::endl;
    }
    dfa_meta dfa = create_full_dfa(std::move(rules));
    // std::cout << dfa.machine << std::endl;
    // std::cout << "trap: " << dfa.trap << std::endl;

    generate_header(out_dir + "/tokens.h", dfa.names);
    generate_cpp(out_dir + "/lexer.cc", dfa.machine, dfa.trap, dfa.names,
                 dfa.final_mapping, dfa.alphabet);
    return 0;
}

dfa_meta create_full_dfa(std::vector<rule> rules) {
    std::map<size_t, std::string> names;
    for (rule &r : rules) {
        names[r.match->id()] = r.name;
    }
    std::vector<std::unique_ptr<ast>> match_seq;
    for (rule &r : rules) {
        match_seq.emplace_back(std::move(r.match));
    }
    auto match = std::make_unique<ast_alt>(std::move(match_seq));
    std::vector<char_range> alphabet;
    std::vector<chr_t> pre_alphabet;
    pre_alphabet.push_back(0);
    match->construct_alphabet(pre_alphabet);
    pre_alphabet.push_back(0x10FFFF + 2);
    std::sort(pre_alphabet.begin(), pre_alphabet.end());
    chr_t current = -1;
    for (chr_t p : pre_alphabet) {
        if (current == -1) {
            current = p;
        } else {
            if (current != p) {
                alphabet.push_back(CHAR_RANGE(current, p));
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
    // std::cout << "nfa: " << machine << std::endl;
    auto [dfa, dead] = machine.powerset(final_mapping, finals);
    // std::cout << "dfa: " << dfa << std::endl;
    return {dfa, dead, finals, final_mapping, alphabet};
}

inline std::ostream &write_line(std::ostream &stream, const char *content,
                                int indent, bool newline = true) {
    for (size_t i = 0; i < indent; i++) {
        stream << "    ";
    }
    stream << content;
    if (newline) {
        stream << std::endl;
    }
    return stream;
}

void generate_cpp(std::string dir, automaton machine, uint16_t trap,
                  std::map<uint16_t, std::string> names,
                  std::map<uint16_t, uint16_t> final_mapping,
                  std::vector<char_range> alphabet) {
    std::ofstream out_code(dir);
    write_line(out_code, "#include <lexer.hh>", 0);
    out_code << std::endl;
    write_line(out_code, "token lexer::next() {", 0);
    write_line(out_code, "uint16_t state = ", 1, false)
        << machine.initial << ";" << std::endl;
    write_line(out_code, "this->m_tk_start = this->stream.pos();", 1);
    write_line(out_code, "while (1) {", 1);
    write_line(out_code, "utf32::chr_t next = this->stream.get();", 2);
    write_line(out_code, "switch (state) {", 2);
    for (uint16_t i = 0; i < machine.states; i++) {
        if (i != trap) {
            write_line(out_code, "case ", 3, false) << i << ":" << std::endl;
            write_line(out_code, "switch (next) {", 4);
            bool is_final = final_mapping.find(i) != final_mapping.end();
            for (size_t a = 1; a <= machine.alphabet; a++) {
                char_range range = alphabet[a - 1];
                chr_t r_start = range >> 32;
                chr_t r_end = range - 1;
                uint16_t next_state = *machine.get(i, a).begin();
                if (!is_final || next_state != trap) {
                    write_line(out_code, "case ", 5, false);
                    if (r_start != r_end) {
                        out_code << r_start << " ... " << r_end << ":"
                                 << std::endl;
                    } else {
                        out_code << r_end << ":" << std::endl;
                    }
                    write_line(out_code, "state = ", 6, false)
                        << next_state << ";" << std::endl;
                    write_line(out_code, "break;", 6);
                }
            }
            if (is_final) {
                write_line(out_code, "default:", 5);
                write_line(out_code, "this->stream.back();", 6);
                write_line(out_code,
                           "this->m_tk_length = this->stream.pos() - "
                           "this->m_tk_start;",
                           6);
                write_line(out_code, "return token::", 6, false)
                    << names[final_mapping[i]] << ";" << std::endl;
            } else {
                write_line(out_code, "case 0xFFFFFFFF:", 5);
                write_line(out_code, "return token::ERROR;", 6);
                write_line(out_code, "default:", 5);
                write_line(out_code, "state = ", 6, false)
                    << trap << ";" << std::endl;
                write_line(out_code, "break;", 6);
            }
            write_line(out_code, "}", 4);
            write_line(out_code, "break;", 4);
        }
    }
    write_line(out_code, "default:", 3);
    write_line(out_code, "return token::ERROR;", 4);
    write_line(out_code, "}", 2);
    write_line(out_code, "}", 1);
    write_line(out_code, "return token::ERROR;", 1);
    write_line(out_code, "}", 0);
    out_code.close();
}

void generate_header(std::string dir, std::map<uint16_t, std::string> names) {
    std::ofstream out_header(dir);
    write_line(out_header, "enum token {", 0);
    write_line(out_header, "ERROR,", 1);
    for (auto &pair : names) {
        write_line(out_header, pair.second.c_str(), 1, false)
            << "," << std::endl;
    }
    write_line(out_header, "};", 0);
    out_header.close();
}