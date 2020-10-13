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

dfa_meta create_full_dfa(std::vector<rule> rules) {
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
    return {dfa, dead, finals, final_mapping, alphabet};
}

void generate_cpp(std::string dir, automaton machine, uint16_t trap,
                  std::map<uint16_t, std::string> names,
                  std::map<uint16_t, uint16_t> final_mapping,
                  std::vector<char_range> alphabet) {
    std::ofstream out_code(dir);
    out_code << "#include <lexer.hh>\n\ntoken lexer::next() {" << std::endl;
    out_code << "uint16_t state = " << machine.initial << ";" << std::endl
             << "this->m_tk_start = this->stream.pos();" << std::endl
             << "while (1) {" << std::endl
             << "utf32::chr_t next = this->stream.get();" << std::endl
             << "switch (state) {" << std::endl;
    // << "std::cout << state << std::endl;" << std::endl
    for (uint16_t i = 0; i < machine.states; i++) {
        if (i != trap) {
            out_code << "case " << i << ":" << std::endl;
            out_code << "switch (next) {" << std::endl;
            bool is_final = final_mapping.find(i) != final_mapping.end();
            for (size_t a = 1; a <= machine.alphabet; a++) {
                char_range range = alphabet[a - 1];
                chr_t r_start = range >> 32;
                chr_t r_end = range - 1;
                uint16_t next_state = *machine.get(i, a).begin();
                if (!is_final || next_state != trap) {
                    if (r_start != r_end) {
                        out_code << "case " << r_start << " ... " << r_end
                                 << ":" << std::endl;
                    } else {
                        out_code << "case " << r_end << ":" << std::endl;
                    }
                    out_code << "state = " << next_state << ";" << std::endl;
                    out_code << "break;" << std::endl;
                }
            }
            if (is_final) {
                out_code << "default:" << std::endl;
                out_code << "this->stream.back();" << std::endl
                         << "this->m_tk_length = this->stream.pos() - "
                            "this->m_tk_start;"
                         << std::endl;
                out_code << "return token::" << names[final_mapping[i]] << ";"
                         << std::endl;
            } else {
                out_code << "default:" << std::endl;
                out_code << "state = " << trap << ";" << std::endl;
                out_code << "break;" << std::endl;
            }
            out_code << "}" << std::endl;
            out_code << "break;" << std::endl;
        }
    }
    out_code << "default: return token::ERROR; }}return token::ERROR;}";
    out_code.close();
}

void generate_header(std::string dir, std::map<uint16_t, std::string> names) {
    std::ofstream out_header(dir);
    out_header << "enum token {" << std::endl;
    out_header << "    ERROR," << std::endl;
    for (auto &pair : names) {
        out_header << "    " << pair.second << "," << std::endl;
    }
    out_header << "};";
    out_header.close();
}