#include <string>
#include <iostream>
#include <vector>
#include <memory>

typedef uint32_t chr_t;

class ast {
   public:
    virtual ~ast();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_char : public ast {
    chr_t ch;

   public:
    ast_char(chr_t ch);
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
    virtual ~ast_class();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_concat : public ast {
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_concat(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
    virtual ~ast_concat();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_alt : public ast {
    std::unique_ptr<ast> child_a;
    std::unique_ptr<ast> child_b;

   public:
    ast_alt(std::unique_ptr<ast> child_a, std::unique_ptr<ast> child_b);
    virtual ~ast_alt();
    virtual std::ostream &print(std::ostream &stream);
};

class ast_rep : public ast {
    std::unique_ptr<ast> child;

   public:
    ast_rep(std::unique_ptr<ast> child);
    virtual ~ast_rep();
    virtual std::ostream &print(std::ostream &stream);
};

struct rule {
    std::string name;
    std::unique_ptr<ast> match;
};

std::vector<rule> read_rules(std::istream &stream);