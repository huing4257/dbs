cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(dbs)


if (APPLE)
    include_directories(
            macos_runtime/usr/local/include/antlr4-runtime/
    )

    link_directories(
            macos_runtime/usr/local/lib/
    )
elseif (UNIX)
    include_directories(
            linux_runtime/usr/local/include/antlr4-runtime/
    )

    link_directories(
            linux_runtime/usr/local/lib/
    )
endif ()

include_directories(
        include
)

set(SOURCES_STAGE
        src/main.cpp
        include/parser/SQLBaseVisitor.cpp
        include/parser/SQLLexer.cpp
        include/parser/SQLParser.cpp
        src/SQLVisitor.cpp
        src/SQLVisitor.cpp src/visitor.cpp include/parser/visitor.h src/visitor.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_STAGE})

target_link_libraries(${PROJECT_NAME} antlr4-runtime)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

