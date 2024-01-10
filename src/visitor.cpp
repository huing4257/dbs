//
// Created by 胡 on 2023/12/30.
//
#include "parser/visitor.h"
#include "utils/error.h"
#include "utils/output.h"
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
        throw std::runtime_error("DB ALREADY EXISTS");
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
        throw Error("DB DOESN'T EXIST");
    }
    return visitChildren(context);
}

std::any Visitor::visitShow_dbs(SQLParser::Show_dbsContext *context) {
    output_sys.output({{"DATABASES"}});
    for (auto &db: databases) {
        output_sys.output({{db.name}});
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
    throw Error("DB DOESN'T EXIST");
}

std::any Visitor::visitShow_tables(SQLParser::Show_tablesContext *context) {
    if (current_db == nullptr) {
        throw Error("NO DATABASE SELECTED");
    }
    output_sys.output({{"TABLES"}});
    for (auto &table: current_db->tables) {
        output_sys.output({{table.name}});
    }
    return visitChildren(context);
}

std::any Visitor::visitCreate_table(SQLParser::Create_tableContext *context) {
    std::string table_name = context->Identifier()->getText();
    for (auto &table: current_db->tables) {
        if (table.name == table_name) {
            throw Error("TABLE ALREADY EXISTS");
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
    int key_num = (int) ids.size() / 2;
    for (int i = 0; i < key_num; ++i) {
        foreign_key.keys.push_back(ids[2 * i]->getText());
        foreign_key.ref_keys.push_back(ids[2 * i + 1]->getText());
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
    auto table_index = current_db->get_table_index(table_name);
    if (table_index == -1) {
        throw Error("TABLE DOESN'T EXIST");
    }
    auto &table = current_db->tables[table_index];

    output_sys.output({{"Field", "Type", "Null", "Default"}});
    for (auto &field: table.fields) {
        std::vector<std::string> record;
        record.push_back(field.name);
        switch (field.type) {
            case FieldType::INT:
                record.emplace_back("INT");
                break;
            case FieldType::FLOAT:
                record.emplace_back("FLOAT");
                break;
            case FieldType::VARCHAR:
                record.push_back("VARCHAR(" + std::to_string(field.length) + ")");
                break;
        }
        record.emplace_back(field.allow_null ? "YES" : "NO");
        record.emplace_back(field.default_value ? "YES" : "NULL");
        output_sys.output({record});
    }
    string line = "\n";
    if (!table.primary_key.keys.empty()) {
        line += "PRIMARY KEY ";
        if (!table.primary_key.name.empty()) {
            line += table.primary_key.name;
        }
        line += "(";
        for (auto &key: table.primary_key.keys) {
            line += (key + ",");
        }
        line.pop_back();
        line += ");";
    }
    line += "\n\n";
    if (!table.foreign_keys.empty()) {
        for (auto &foreign_key: table.foreign_keys) {
            line += "FOREIGN KEY ";
            if (!foreign_key.name.empty()) {
                line += foreign_key.name;
            }
            line += "(";
            for (auto &key: foreign_key.keys) {
                line += (key + ",");
            }
            line.pop_back();
            line +=  ") REFERENCES " + foreign_key.table_name + "(";
            for (auto &key: foreign_key.ref_keys) {
                line += (key + ",");
            }
            line.pop_back();
            line += ");\n";
        }
    }
    output_sys.output({{line}});
    return {};
}
std::any Visitor::visitLoad_table(SQLParser::Load_tableContext *context) {
    auto table_name = context->Identifier()->getText();
    //    auto &table = current_db->get_table(table_name);
    auto file_name = context->String()[0]->getText();
    auto terminator = context->String()[1]->getText()[0];
    ifstream file(file_name);
    if (!file.is_open()) throw std::runtime_error("FILE NOT FOUND");
    // decide whether to alloc new page
    int count = 0;
    string line;
    vector<vector<Value>> records;
    while (getline(file, line)) {
        string tmp;
        stringstream ss(line);
        vector<string> str_record;
        while (getline(ss, tmp, terminator)) {
            str_record.push_back(tmp);
        }


        count++;
    }
    return {};
}
