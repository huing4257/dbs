cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(dbs)

option(ASANoff "EnableASAN" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
if (ASAN)
    include_directories(
            asan_run/usr/local/include/antlr4-runtime/
    )

    link_directories(
            asan_run/usr/local/lib/
    )
elseif (APPLE)
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
        src/visitor.cpp
        src/database.cpp
        include/utils/error.h
        src/output.cpp
        include/utils/output.h
        src/Selector.cpp
        include/utils/Selector.h
)

add_executable(${PROJECT_NAME} ${SOURCES_STAGE})

target_link_libraries(${PROJECT_NAME} antlr4-runtime)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

if (ASAN)
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall
            -fsanitize=address
            -fno-omit-frame-pointer
            -fsanitize=undefined
    )

    target_link_options(${PROJECT_NAME} PRIVATE
            -fsanitize=address
            -fno-omit-frame-pointer
            -fsanitize=undefined
    )
endif (ASAN)