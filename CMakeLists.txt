cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project (dbs)

find_package(antlr4-runtime REQUIRED)

include_directories(
  ${antlr4-runtime_INCLUDE_DIR}
  src/parser
)

set(SOURCES_STAGE
    src/main.cpp
    src/parser/SQLLexer.cpp
    src/parser/SQLParser.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCES_STAGE} )

target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS} antlr4_static -pthread )

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

