#pragma once

#include <string>
#include <memory>

#include "automaton.hh"

typedef uint32_t chr_t;
typedef uint64_t char_range;

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

class ast_char : public ast {
    chr_t ch;

   public:
    ast_char(chr_t ch);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_char();
    virtual std::ostream &print(std::ostream &stream);
};

enum regex_class {
    WORD,
    DIGIT,
    SPACE,
};

class ast_class : public ast {
    regex_class cls;

   public:
    ast_class(regex_class cls);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_class();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_cat : public ast {
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_cat(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
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
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_alt(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
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

   public:
    ast_rep(std::unique_ptr<ast> child);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     std::map<size_t, std::string> &names,
                                     std::map<uint16_t, std::string> &finals,
                                     uint16_t *state_count);
    virtual void construct_alphabet(std::vector<chr_t> &alphabet);
    virtual ~ast_rep();
    virtual std::ostream &print(std::ostream &stream);
};