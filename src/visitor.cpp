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
                        }),
                        databases.end());
    } catch (std::exception &e) {
        std::cout << "@DB DOESN'T EXIST" << e.what();
    }
    return visitChildren(context);
}

std::any Visitor::visitShow_dbs(SQLParser::Show_dbsContext *context) {
    cout << "DATABASES" << endl;
    for (auto &db: databases) {
        cout << db.name << endl;
    }
    return visitChildren(context);
}

std::any Visitor::visitUse_db(SQLParser::Use_dbContext *context) {
    std::string db_name = context->Identifier()->getText();
    if (current_db != nullptr) {
        current_db->close_database();
    }
    for (auto &db: databases) {
        if (db.name == db_name) {
            db.use_database();
            current_db = &db;
            return {};
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
    return visitChildren(context);
}

std::any Visitor::visitCreate_table(SQLParser::Create_tableContext *context) {
    std::string table_name = context->Identifier()->getText();
    for (auto &table: current_db->tables) {
        if (table.name == table_name) {
            std::cout << "@TABLE ALREADY EXISTS";
            return {};
        }
    }
    auto field_list = context->field_list()->accept(this);
    Table table;
    table.name = table_name;
    table.fields.clear();
    for (auto &field: context->field_list()->field()) {
        if (typeid(*field) == typeid(SQLParser::Primary_key_fieldContext)) {
            auto new_field = std::any_cast<PrimaryKey>(field->accept(this));
            table.primary_key = new_field;
            continue;
        }
        if (typeid(*field) == typeid(SQLParser::Foreign_key_fieldContext)) {
            auto new_field = std::any_cast<ForeignKey>(field->accept(this));
            table.foreign_keys.push_back(new_field);
            continue;
        }
        auto new_field = std::any_cast<Field>(field->accept(this));
        table.fields.push_back(new_field);
    }
    table.construct();
    current_db->create_open_table(table);
    return {};
}

std::any Visitor::visitNormal_field(SQLParser::Normal_fieldContext *context) {
    Field field;
    field.name = context->Identifier()->getText();
    auto type = context->type_()->getStart()->getText();
    if (type == "INT") {
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
    if (context->Null()) {
        field.allow_null = false;
    }
    return field;
}

std::any Visitor::visitPrimary_key_field(SQLParser::Primary_key_fieldContext *context) {
    PrimaryKey primary_key;
    auto id = context->Identifier();
    if (id) {
        primary_key.name = id->getText();
    }
    auto ids = context->identifiers()->Identifier();
    for (auto &name: ids) {
        primary_key.keys.push_back(name->getText());
    }
    return primary_key;
}

std::any Visitor::visitForeign_key_field(SQLParser::Foreign_key_fieldContext *context) {
    ForeignKey foreign_key;
    auto id = context->Identifier();
    if (id.size() == 1) {
        foreign_key.name = "";
        foreign_key.table_name = id[0]->getText();
    } else {
        foreign_key.name = id[0]->getText();
        foreign_key.table_name = id[1]->getText();
    }
    auto ids = context->identifiers();
    int key_num = (int)ids.size()/2;
    for (int i = 0; i < key_num; ++i) {
        foreign_key.keys.push_back(ids[2*i]->getText());
        foreign_key.ref_keys.push_back(ids[2*i+1]->getText());
    }
    return foreign_key;
}


std::any Visitor::visitDrop_table(SQLParser::Drop_tableContext *context) {
    std::string table_name = context->Identifier()->getText();
    current_db->drop_table(table_name);
    return {};
}

std::any Visitor::visitDescribe_table(SQLParser::Describe_tableContext *context) {
    std::string table_name = context->Identifier()->getText();
    for (auto &table: current_db->tables) {
        if (table.name == table_name) {
            cout << "Field,Type,Null,Default" << endl;
            for (auto &field: table.fields) {
                cout << field.name << ",";
                switch (field.type) {
                    case FieldType::INT:
                        cout << "INT,";
                        break;
                    case FieldType::VARCHAR:
                        cout << "VARCHAR(" << field.length << "),";
                        break;
                    case FieldType::FLOAT:
                        cout << "FLOAT,";
                        break;
                }
                cout << (field.allow_null ? "YES" : "NO") << ",";
                cout << (field.default_value.has_value() ? " " : "NULL") << endl;
            }
            cout << endl;
            if (!table.primary_key.keys.empty()) {
                cout << "PRIMARY KEY ";
                if (!table.primary_key.name.empty()) {
                    cout << table.primary_key.name;
                }
                cout << "(";
                string tmp;
                for (auto &key: table.primary_key.keys) {
                    tmp += (key + ",");
                }
                tmp.pop_back();
                cout << tmp;
                cout << ");" << endl;
            }
            if (!table.foreign_keys.empty()) {
                for (auto &foreign_key: table.foreign_keys) {
                    cout << "FOREIGN KEY ";
                    if (!foreign_key.name.empty()) {
                        cout << foreign_key.name;
                    }
                    cout << "(";
                    string tmp;
                    for (auto &key: foreign_key.keys) {
                        tmp += (key + ",");
                    }
                    tmp.pop_back();
                    cout << tmp;
                    cout << ") REFERENCES " << foreign_key.table_name << "(";
                    tmp = "";
                    for (auto &key: foreign_key.ref_keys) {
                        tmp += (key + ",");
                    }
                    tmp.pop_back();
                    cout << tmp;
                    cout << ");" << endl;
                }
            }
            return {};
        }
    }
    std::cout << "@TABLE DOESN'T EXIST";
    return {};
}
std::any Visitor::visitLoad_table(SQLParser::Load_tableContext *context) {
    auto table_name = context->Identifier()->getText();
    auto file_name = context->String()[0]->getText();
    auto terminator = context->String()[1]->getText();
    return std::any();
}
