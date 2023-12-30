#include <iostream>
#include <cstdio>
#include "antlr4-runtime.h"
#include <sys/stat.h>
#include "dbsystem/init.h"
#include "parser/SQLLexer.h"
#include "parser/visitor.h"

using namespace std;

int main() {
    // create directory structure
    init_dir();
    // main loop
    while (true) {
        std::string line;
        std::getline(std::cin, line);
        if (line == "exit") {
            break;
        }
        antlr4::ANTLRInputStream input(line);
        SQLLexer lexer(&input);
        antlr4::CommonTokenStream tokens(&lexer);
        SQLParser parser(&tokens);
        auto tree = parser.program();
//        cout << tree->toStringTree(&parser) << endl;
        auto visitor = Visitor();
        visitor.visit(tree);
        std::cout << "@" << line << std::endl;
    }
    return 0;
}