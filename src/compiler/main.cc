#include "parser.hh"

#include <fstream>
#include <iostream>

int main(int argc, char const *argv[]) {
    std::string path = "../test.sp";
    std::ifstream in_file(path);
    if (!in_file.is_open()) {
        throw std::runtime_error("unable to open file: " + path);
    }
    lexer my_lexer(in_file);
    token t = my_lexer.next();
    std::cout << t << std::endl;
    return 0;
}
