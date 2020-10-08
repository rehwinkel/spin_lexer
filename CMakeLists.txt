cmake_minimum_required(VERSION 2.8)
project(spin)
set(CMAKE_BUILD_TYPE Debug)

add_executable(spinc src/compiler/main.cc src/compiler/parser.cc src/compiler/lexer.cc src/compiler/utf32.cc)