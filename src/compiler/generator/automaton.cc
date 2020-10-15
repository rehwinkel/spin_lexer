#include "automaton.hh"

std::ostream &print_set(std::ostream &stream, std::set<uint16_t> states) {
    stream << "{ ";
    for (uint16_t el : states) {
        stream << el << " ";
    }
    stream << "}";
    return stream;
}

automaton::automaton(uint16_t states, std::set<uint16_t> finals,
                     uint32_t alphabet, uint16_t initial)
    : states(states), initial(initial), alphabet(alphabet), finals(finals) {}

void automaton::connect(uint16_t start, uint16_t end, uint32_t input) {
    uint64_t id = ((uint64_t)start) << 48 | ((uint64_t)end) << 32 | input;
    this->transition[id] = end;
}

void automaton::_epsilon_closure_rec(std::set<uint16_t> &closure,
                                     uint16_t state) {
    closure.insert(state);
    for (uint16_t state : this->get(state, 0)) {
        if (!closure.contains(state)) {
            automaton::_epsilon_closure_rec(closure, state);
        }
    }
}

std::set<uint16_t> automaton::epsilon_closure(uint16_t state) {
    std::set<uint16_t> result;
    this->_epsilon_closure_rec(result, state);
    return result;
}

std::set<uint16_t> automaton::input_closure(std::set<uint16_t> &state_e_closure,
                                            uint32_t input) {
    std::set<uint16_t> result;
    for (uint16_t state : state_e_closure) {
        for (uint16_t state2 : this->get(state, input)) {
            this->_epsilon_closure_rec(result, state2);
        }
    }
    return result;
}

uint16_t automaton::get(uint16_t start, uint16_t end, uint32_t input) {
    uint64_t id = ((uint64_t)start) << 48 | ((uint64_t)end) << 32 | input;
    return this->transition[id];
}

std::set<uint16_t> automaton::get(uint16_t start, uint32_t input) {
    std::set<uint16_t> states;
    uint64_t id = ((uint64_t)start) << 48 | input;
    for (auto &pair : this->transition) {
        if ((pair.first & 0xffff0000ffffffff) == id) {
            states.insert(pair.second);
        }
    }
    return states;
}

std::ostream &operator<<(std::ostream &stream, const automaton &el) {
    stream << "Automaton(states=" << el.states << ", initial=" << el.initial + 1
           << ", alphabet=" << el.alphabet << ", finals={ ";
    for (uint16_t state : el.finals) {
        stream << state + 1 << " ";
    }
    stream << "}, connections=[ ";
    for (auto &pair : el.transition) {
        uint16_t start = pair.first >> 48;
        uint16_t end = pair.second;
        uint32_t input = pair.first;
        stream << "(" << start + 1 << ")--[" << input - 1 << "]->(" << end + 1
               << ") ";
    }
    stream << "])";
    return stream;
}

void automaton::find_state_sets(std::vector<std::set<uint16_t>> &state_sets,
                                std::map<uint64_t, uint16_t> &new_transition,
                                std::set<uint16_t> origin) {
    if (std::find(state_sets.begin(), state_sets.end(), origin) ==
        state_sets.end()) {
        state_sets.push_back(origin);
        for (uint32_t input = 1; input <= this->alphabet; input++) {
            auto closure = this->input_closure(origin, input);
            this->find_state_sets(state_sets, new_transition, closure);

            uint16_t origin_id =
                std::find(state_sets.begin(), state_sets.end(), origin) -
                state_sets.begin();
            uint16_t closure_id =
                std::find(state_sets.begin(), state_sets.end(), closure) -
                state_sets.begin();
            uint64_t id = ((uint64_t)origin_id) << 48 |
                          ((uint64_t)closure_id) << 32 | input;
            new_transition[id] = closure_id;
        }
    }
}

std::pair<automaton, uint16_t> automaton::powerset(
    std::map<uint16_t, uint16_t> &final_mapping,
    const std::map<uint16_t, std::string> &names) {
    std::vector<std::set<uint16_t>> state_sets;
    std::map<uint64_t, uint16_t> new_transition;
    auto initial_closure = this->epsilon_closure(this->initial);
    this->find_state_sets(state_sets, new_transition, initial_closure);
    std::vector<uint16_t> new_finals;
    for (std::set<uint16_t> &state_set : state_sets) {
        std::vector<uint16_t> tmp_finals_inters;
        std::set_intersection(state_set.begin(), state_set.end(),
                              this->finals.begin(), this->finals.end(),
                              std::back_inserter(tmp_finals_inters));
        if (tmp_finals_inters.size() > 0) {
            uint16_t state =
                std::find(state_sets.begin(), state_sets.end(), state_set) -
                state_sets.begin();
            new_finals.push_back(state);
            for (uint16_t orig_final : tmp_finals_inters) {
                auto &mapping = final_mapping[state];
                if (mapping != 0) {
                    std::cout << "Overwriting state result of "
                              << names.at(mapping) << " with "
                              << names.at(orig_final) << std::endl;
                }
                mapping = orig_final;
            }
        }
    }
    automaton resulting(
        state_sets.size(),
        std::set<uint16_t>(new_finals.begin(), new_finals.end()),
        this->alphabet,
        std::find(state_sets.begin(), state_sets.end(), initial_closure) -
            state_sets.begin());
    for (auto &pair : new_transition) {
        uint16_t start = pair.first >> 48;
        uint16_t end = pair.first >> 32;
        uint32_t input = pair.first;
        resulting.connect(start, end, input);
    }
    return std::make_pair(
        resulting,
        std::find(state_sets.begin(), state_sets.end(), std::set<uint16_t>{}) -
            state_sets.begin());
}

/*
int main(int argc, char const *argv[]) {
    automaton a(4, std::set<uint16_t>{2, 3}, 2, 0);
    a.connect(0, 2, 0);
    a.connect(0, 1, 1);
    a.connect(1, 1, 2);
    a.connect(1, 3, 2);
    a.connect(2, 1, 0);
    a.connect(2, 3, 1);
    a.connect(3, 2, 1);
    automaton b = a.powerset();
    std::cout << b << std::endl;
    return 0;
}
*/

/*
int main(int argc, char const *argv[]) {
    automaton a(10, std::set<uint16_t>{9}, 3, 8);
    a.connect(8, 0, 0);
    a.connect(8, 9, 0);
    a.connect(0, 1, 0);
    a.connect(0, 5, 0);
    a.connect(1, 2, 1);
    a.connect(2, 3, 2);
    a.connect(3, 4, 0);
    a.connect(4, 9, 0);
    a.connect(4, 0, 0);
    a.connect(5, 6, 1);
    a.connect(6, 7, 3);
    a.connect(7, 4, 0);
    automaton b = a.powerset();
    std::cout << b << std::endl;
    return 0;
}
*/