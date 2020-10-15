#include <set>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <vector>
#include <algorithm>

std::ostream &print_set(std::ostream &stream,
                        std::unordered_set<uint16_t> states);

class automaton {
    std::set<uint16_t> epsilon_closure(uint16_t state);
    std::set<uint16_t> input_closure(std::set<uint16_t> &state_e_closure,
                                     uint32_t input);
    void _epsilon_closure_rec(std::set<uint16_t> &closure, uint16_t state);
    void find_state_sets(std::vector<std::set<uint16_t>> &state_sets,
                         std::unordered_map<uint64_t, uint16_t> &new_transition,
                         std::set<uint16_t> &origin);

   public:
    uint16_t states, initial;
    uint32_t alphabet;
    std::unordered_set<uint16_t> finals;
    std::unordered_map<uint64_t, uint16_t> transition;
    automaton(uint16_t states, std::unordered_set<uint16_t> finals,
              uint32_t alphabet, uint16_t initial);
    void connect(uint16_t start, uint16_t end, uint32_t input);
    uint16_t get(uint16_t start, uint16_t end, uint32_t input);
    std::pair<automaton, uint16_t> powerset(
        std::unordered_map<uint16_t, uint16_t> &final_mapping,
        const std::unordered_map<uint16_t, std::string> &names);
    friend std::ostream &operator<<(std::ostream &stream, const automaton &el);
};