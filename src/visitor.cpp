//
// Created by 胡 on 2023/12/30.
//
#include "parser/visitor.h"
#include <filesystem>
using namespace std;

std::any Visitor::visitProgram(SQLParser::ProgramContext *context) {
    return visitChildren(context);
}

std::any Visitor::visitStatement(SQLParser::StatementContext *context) {
    return visitChildren(context);
}

std::any Visitor::visitCreate_db(SQLParser::Create_dbContext *context) {
    std::string db_name = context->Identifier()->getText();
    try {
        filesystem::create_directory("data/base/" + db_name);
    } catch (std::exception &e) {
        std::cout << "@DB ALREADY EXISTS" << std::endl;
    }

    return visitChildren(context);
}

std::any Visitor::visitDrop_db(SQLParser::Drop_dbContext *context) {
    std::string db_name = context->Identifier()->getText();
    try {
        filesystem::remove_all("data/base/" + db_name);
    } catch (std::exception &e) {
        std::cout << "@DB DOESN'T EXIST" << e.what() << std::endl;
    }
    return visitChildren(context);
}

std::any Visitor::visitShow_dbs(SQLParser::Show_dbsContext *context) {
    cout << "DATABASES" << endl;
    for (const auto &entry: std::filesystem::directory_iterator("data/base")) {
        std::cout << entry.path().filename().string() << std::endl;
    }
    return visitChildren(context);
}