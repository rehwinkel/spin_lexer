#pragma once

#include <string>
#include <memory>

#include "automaton.hh"

typedef uint32_t chr_t;
typedef uint64_t char_range;

char_range make_char_range(chr_t start, chr_t end);

struct autopart {
    uint16_t start;
    uint16_t end;
};

static size_t ast_counter = 0;

class ast {
    size_t index;

   public:
    ast();
    size_t id();
    virtual ~ast();
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count) = 0;
    virtual void construct_alphabet(std::vector<chr_t> &alphabet) = 0;
    virtual std::ostream &print(std::ostream &stream);
};

class ast_set : public ast {
    std::vector<char_range> ranges;
    bool negate;

   public:
    ast_set(chr_t start, chr_t end, bool negate);
    ast_set(std::vector<char_range>, bool negate);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_set();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_cat : public ast {
    std::vector<std::unique_ptr<ast>> children;

   public:
    ast_cat(std::vector<std::unique_ptr<ast>> children);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_cat();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_alt : public ast {
    std::vector<std::unique_ptr<ast>> children;

   public:
    ast_alt(std::vector<std::unique_ptr<ast>> children);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_alt();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_rep : public ast {
    std::unique_ptr<ast> child;
    bool accept_empty;

   public:
    ast_rep(std::unique_ptr<ast> child, bool accept_empty);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_rep();
    virtual std::ostream &print(std::ostream &stream);
};