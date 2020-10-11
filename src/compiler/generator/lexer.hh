#include <string>
#include <memory>

#include "automaton.hh"

typedef uint32_t chr_t;

struct autopart {
    uint16_t start;
    uint16_t end;
};

typedef uint64_t char_range;

char_range make_char_range(chr_t start, chr_t end);

class ast {
   public:
    virtual ~ast();
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count) = 0;
    virtual std::ostream &print(std::ostream &stream);
};

class ast_char : public ast {
    chr_t ch;

   public:
    ast_char(chr_t ch);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count);
    virtual ~ast_char();
    virtual std::ostream &print(std::ostream &stream);
};

enum regex_class {
    WORD,
    DIGIT,
    SPACE,
    NOT_WORD,
    NOT_DIGIT,
    NOT_SPACE,
};

class ast_class : public ast {
    regex_class cls;

   public:
    ast_class(regex_class cls);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count);
    virtual ~ast_class();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_concat : public ast {
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_concat(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count);
    virtual ~ast_concat();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_alt : public ast {
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_alt(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count);
    virtual ~ast_alt();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_rep : public ast {
    std::unique_ptr<ast> child;

   public:
    ast_rep(std::unique_ptr<ast> child);
    virtual autopart connect_machine(automaton &machine,
                                     std::vector<char_range> &alphabet,
                                     uint16_t *state_count);
    virtual ~ast_rep();
    virtual std::ostream &print(std::ostream &stream);
};

struct rule {
    std::string name;
    std::unique_ptr<ast> match;
};

std::vector<rule> read_rules(std::istream &stream);

automaton create_nfa(std::vector<rule> rules);