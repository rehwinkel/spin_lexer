cmake_minimum_required(VERSION 2.8)
project(spin)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(src/compiler/generator)

include_directories(${PROJECT_BINARY_DIR} src/compiler)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/lexer.cc ${PROJECT_BINARY_DIR}/tokens.h COMMAND lexergen ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/src/compiler/lexer.rules DEPENDS src/compiler/lexer.rules lexergen)

add_executable(spinc src/compiler/main.cc src/compiler/parser.cc src/compiler/lexer.cc src/compiler/utf32.cc ${PROJECT_BINARY_DIR}/lexer.cc)