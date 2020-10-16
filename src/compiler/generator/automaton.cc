#include "automaton.hh"

std::set<uint16_t> intersect_set(std::unordered_set<uint16_t> &set_a,
                                 std::unordered_set<uint16_t> &set_b) {
    std::set<uint16_t> output;
    for (uint16_t el : set_a) {
        if (set_b.contains(el)) {
            output.insert(el);
        }
    }
    return std::move(output);
}

automaton::automaton(uint16_t states, std::unordered_set<uint16_t> finals,
                     uint32_t alphabet, uint16_t initial)
    : states(states), initial(initial), alphabet(alphabet), finals(finals) {}

void automaton::connect(uint16_t start, uint16_t end, uint32_t input) {
    uint64_t id = ((uint64_t)start) << 48 | ((uint64_t)end) << 32 | input;
    this->transition[id] = end;
}

void automaton::_epsilon_closure_rec(std::unordered_set<uint16_t> &closure,
                                     uint16_t state) {
    closure.insert(state);
    uint64_t id = ((uint64_t)state) << 48;
    for (auto &pair : this->transition) {
        if ((pair.first & 0xffff0000ffffffff) == id) {
            if (!closure.contains(pair.second)) {
                automaton::_epsilon_closure_rec(closure, pair.second);
            }
        }
    }
}

std::unordered_set<uint16_t> automaton::epsilon_closure(uint16_t state) {
    std::unordered_set<uint16_t> result;
    this->_epsilon_closure_rec(result, state);
    return result;
}

std::unordered_set<uint16_t> automaton::input_closure(
    std::unordered_set<uint16_t> &state_e_closure, uint32_t input) {
    std::unordered_set<uint16_t> result;
    for (uint16_t state : state_e_closure) {
        uint64_t id = ((uint64_t)state) << 48 | input;
        for (auto &pair : this->transition) {
            if ((pair.first & 0xffff0000ffffffff) == id) {
                this->_epsilon_closure_rec(result, pair.second);
            }
        }
    }
    return result;
}

uint16_t automaton::get(uint16_t start, uint16_t end, uint32_t input) {
    uint64_t id = ((uint64_t)start) << 48 | ((uint64_t)end) << 32 | input;
    return this->transition[id];
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

void automaton::find_state_sets(
    std::vector<std::unordered_set<uint16_t>> &state_sets,
    std::unordered_map<uint64_t, uint16_t> &new_transition,
    std::unordered_set<uint16_t> &origin) {
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
    std::unordered_map<uint16_t, uint16_t> &final_mapping,
    const std::unordered_map<uint16_t, std::string> &names) {
    std::vector<std::unordered_set<uint16_t>> state_sets;
    std::unordered_map<uint64_t, uint16_t> new_transition;
    auto initial_closure = this->epsilon_closure(this->initial);
    this->find_state_sets(state_sets, new_transition, initial_closure);
    std::vector<uint16_t> new_finals;
    for (std::unordered_set<uint16_t> &state_set : state_sets) {
        std::set<uint16_t> tmp_finals_inters =
            intersect_set(state_set, this->finals);
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
        std::unordered_set<uint16_t>(new_finals.begin(), new_finals.end()),
        this->alphabet,
        std::find(state_sets.begin(), state_sets.end(), initial_closure) -
            state_sets.begin());
    for (auto &pair : new_transition) {
        uint16_t start = pair.first >> 48;
        uint16_t end = pair.first >> 32;
        uint32_t input = pair.first;
        resulting.connect(start, end, input);
    }
    return std::make_pair(resulting,
                          std::find(state_sets.begin(), state_sets.end(),
                                    std::unordered_set<uint16_t>{}) -
                              state_sets.begin());
}