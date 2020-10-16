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
    std::unordered_map<size_t, std::string> names;
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

    std::unordered_map<uint16_t, std::string> finals;
    automaton machine(0, std::unordered_set<uint16_t>{}, 0, 0);
    uint16_t state_count = 0;
    autopart part =
        match->connect_machine(machine, alphabet, names, finals, &state_count);
    machine.states = state_count;
    machine.alphabet = alphabet.size();
    std::unordered_set<uint16_t> actual_finals;
    for (auto &fin : finals) {
        actual_finals.insert(fin.first);
    }
    machine.finals = actual_finals;
    std::unordered_map<uint16_t, uint16_t> final_mapping;
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
                  std::unordered_map<uint16_t, std::string> names,
                  std::unordered_map<uint16_t, uint16_t> final_mapping,
                  std::vector<char_range> alphabet) {
    std::ofstream out_code(dir);
    out_code << "#include <lexer.hh>" << std::endl
             << "token lexer::next(){uint16_t s=" << machine.initial
             << ";this->m_tk_start=this->stream.pos();while(1){utf32::chr_t n="
                "this->stream.get();switch(s){";
    for (uint16_t i = 0; i < machine.states; i++) {
        if (i != trap) {
            out_code << "case " << i << ":switch(n){";
            bool is_final = final_mapping.find(i) != final_mapping.end();
            for (size_t a = 1; a <= machine.alphabet; a++) {
                char_range range = alphabet[a - 1];
                chr_t r_start = range >> 32;
                chr_t r_end = range - 1;
                uint16_t next_state;
                uint64_t id = ((uint64_t)i) << 48 | a;
                for (auto &pair : machine.transition) {
                    if ((pair.first & 0xffff0000ffffffff) == id) {
                        next_state = pair.second;
                        break;
                    }
                }
                if (!is_final || next_state != trap) {
                    out_code << "case ";
                    if (r_start != r_end) {
                        out_code << r_start << " ... " << r_end << ":";
                    } else {
                        out_code << r_end << ":";
                    }
                    out_code << "s=" << next_state << ";break;";
                }
            }
            if (is_final) {
                out_code << "default:this->stream.back();this->m_tk_length="
                            "this->stream.pos()-this->m_tk_start;return token::"
                         << names[final_mapping[i]] << ";";
            } else {
                out_code << "case 0xFFFFFFFF:return token::ERROR;default:s="
                         << trap << ";break;";
            }
            out_code << "}break;";
        }
    }
    out_code << "default:return token::ERROR;}}return token::ERROR;}";
    out_code.close();
}

void generate_header(std::string dir,
                     std::unordered_map<uint16_t, std::string> names) {
    std::ofstream out_header(dir);
    out_header << "enum token {" << std::endl << "    ERROR," << std::endl;
    for (auto &pair : names) {
        out_header << "    " << pair.second.c_str() << "," << std::endl;
    }
    out_header << "};";
    out_header.close();
}