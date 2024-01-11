#include "antlr4-runtime.h"
#include "dbsystem/init.h"
#include "parser/SQLLexer.h"
#include "parser/visitor.h"
#include "utils/error.h"
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <sys/stat.h>
#include "utils/output.h"

using namespace std;
Output output_sys = Output(OutputType::BATCH);

int main(int argc, char *argv[]) {
    // create directory structure
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--init") == 0) {
            filesystem::remove_all("data");
            init_dir();
            return 0;
        }
    }
    // init database
//    init_dir();
    init_database();
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
        try {
            visitor.visit(tree);
        } catch (const Error &e) {
            cout << "!ERROR " << e.what() << endl;
        }
        if (current_db != nullptr) {
            cout << "@" << "DB:" << current_db->name << " ";
        }
        cout << "@" <<line << endl;
    }
    return 0;
}