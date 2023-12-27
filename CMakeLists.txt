cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project (dbs)

if(APPLE)
  include_directories(
    macos_runtime/usr/local/include/antlr4-runtime/
  )

  link_directories(
    macos_runtime/usr/local/lib/
  )
elseif(UNIX)
  include_directories(
    linux_runtime/usr/local/include/antlr4-runtime/
  )

  link_directories(
    linux_runtime/usr/local/lib/
  )
endif()

set(SOURCES_STAGE
    src/main.cpp
    src/parser/SQLLexer.cpp
    src/parser/SQLParser.cpp
    )

add_executable(${PROJECT_NAME} ${SOURCES_STAGE} )

target_link_libraries(${PROJECT_NAME} antlr4-runtime)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

