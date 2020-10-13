include_directories(${PROJECT_SOURCE_DIR}/src/compiler)

add_executable(lexergen lexer.cc ast.cc rules.cc automaton.cc ${PROJECT_SOURCE_DIR}/src/compiler/utf32.cc)
set_property(TARGET lexergen PROPERTY CXX_STANDARD 20)