//
// Created by 胡 on 2023/12/30.
//
#include "parser/visitor.h"
#include <filesystem>

using namespace std;
Database *current_db = nullptr;

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
    databases.emplace_back(db_name);

    return visitChildren(context);
}

std::any Visitor::visitDrop_db(SQLParser::Drop_dbContext *context) {
    std::string db_name = context->Identifier()->getText();
    try {
        filesystem::remove_all("data/base/" + db_name);
        databases.erase(std::remove_if(databases.begin(), databases.end(), [&db_name](Database &db) {
            return db.name == db_name;
        }), databases.end());
    } catch (std::exception &e) {
        std::cout << "@DB DOESN'T EXIST" << e.what() ;
    }
    return visitChildren(context);
}

std::any Visitor::visitShow_dbs(SQLParser::Show_dbsContext *context) {
    cout << "DATABASES" << endl;
    for(auto &db: databases) {
        cout << db.name << endl;
    }
    return visitChildren(context);
}

std::any Visitor::visitUse_db(SQLParser::Use_dbContext *context) {
    std::string db_name = context->Identifier()->getText();
    for (auto &db: databases) {
        if (db.name == db_name) {
            db.use_database();
            current_db = &db;
            return visitChildren(context);
        }
    }
    std::cout << "@DB DOESN'T EXIST" << std::endl;
    return visitChildren(context);
}

std::any Visitor::visitShow_tables(SQLParser::Show_tablesContext *context) {
    cout << "TABLES" << endl;
    for (auto &table: current_db->tables) {
        cout << table.name << endl;
    }
    std::cout << "@DB DOESN'T EXIST" << std::endl;
    return visitChildren(context);
}

std::any Visitor::visitCreate_table(SQLParser::Create_tableContext *context) {
    std::string table_name = context->Identifier()->getText();
    auto field_list = context->field_list()->accept(this);
    Table table;
    table.name = table_name;
    table.fields = std::any_cast<std::vector<Field>>(field_list);

    return {};
}

std::any Visitor::visitField_list(SQLParser::Field_listContext *context) {
    std::vector<Field> field_list;
    for (auto &field: context->field()) {
        field_list.push_back(std::any_cast<Field>(field->accept(this)));
    }
    return field_list;
}

std::any Visitor::visitNormal_field(SQLParser::Normal_fieldContext *context) {
    Field field;
    field.name = context->Identifier()->getText();
    auto type = context->type_()->getStart()->getText();
    if (type == "INT"){
        field.type = FieldType::INT;
    } else if (type == "VARCHAR") {
        field.type = FieldType::VARCHAR;
        auto value = context->type_()->children[2]->getText();
        field.length = std::stoi(value);
    } else if (type == "FLOAT") {
        field.type = FieldType::FLOAT;
    } else {
        field.type = FieldType::INT;
    }
    return field;
}

std::any Visitor::visitPrimary_key_field(SQLParser::Primary_key_fieldContext *context) {
    Field field;
    auto id = context->Identifier();
    if (id) {
        field.name = id->getText();
    } else {
        field.name = "";
        for (auto &name : context->identifiers()->Identifier()) {
            field.name += name->getText();
            field.name += "_";
        }
        field.name.pop_back();
    }
    field.is_primary_key = true;
    field.type = FieldType::INT;
    return field;
}

std::any Visitor::visitForeign_key_field(SQLParser::Foreign_key_fieldContext *context) {
    Field field;
    field.name = context->Identifier(0)->getText();
    field.is_foreign_key = true;
    field.type = FieldType::INT;
    return field;
}

