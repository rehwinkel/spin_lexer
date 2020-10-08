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

class ast_seq : public ast {
    std::vector<std::unique_ptr<ast>> children;

   public:
    ast_seq(std::vector<std::unique_ptr<ast>> children);
    virtual ~ast_seq();
    virtual std::ostream &print(std::ostream &stream);
};

struct rule {
    std::string name;
    std::unique_ptr<ast> match;
};

std::vector<rule> read_rules(std::istream &stream);