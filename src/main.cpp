#include <iostream>
#include <cstdio>
#include "antlr4-runtime.h"
#include <sys/stat.h>
#include "dbsystem/init.h"
#include "parser/SQLLexer.h"
#include "parser/visitor.h"
#include <filesystem>

using namespace std;

int main(int argc, char *argv[]) {
    // create directory structure
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--init") == 0) {
            filesystem::remove_all("data");
            init_dir();
            return 0;
        }
    }
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
        if (tree->children.size() <= 1) {
            continue;
        }
//        cerr << tree->toStringTree(&parser) << endl;
        auto visitor = Visitor();
        visitor.visit(tree);
        if (current_db != nullptr) {
            cout << "@" << "DB:" << current_db->name << " ";
        }
        cout << "@" <<line << endl;
    }
    return 0;
}