#include "rules.hh"

char_range make_char_range(chr_t start, chr_t end);

struct dfa_meta {
    automaton machine;
    uint16_t trap;
    std::unordered_map<uint16_t, std::string> names;
    std::unordered_map<uint16_t, uint16_t> final_mapping;
    std::vector<char_range> alphabet;
};

dfa_meta create_full_dfa(std::vector<rule> rules);

void generate_header(std::string dir, std::unordered_map<uint16_t, std::string> names);

void generate_cpp(std::string dir, automaton machine, uint16_t trap,
                  std::unordered_map<uint16_t, std::string> names,
                  std::unordered_map<uint16_t, uint16_t> final_mapping,
                  std::vector<char_range> alphabet);