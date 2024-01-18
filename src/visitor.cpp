//
// Created by 胡 on 2023/12/30.
//
#include "parser/visitor.h"
#include "utils/Selector.h"
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
        if (current_db->name == db_name) {
            return {};
        }
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
            line += ") REFERENCES " + foreign_key.table_name + "(";
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
    int table_index = current_db->get_table_index(table_name);
    auto file_name = context->String()[0]->getText();
    auto terminator = context->String()[1]->getText();
    auto &table = current_db->tables[table_index];
    ifstream file(file_name.substr(1, file_name.size() - 2), ios::in);
    if (!file.is_open()) throw Error("FILE NOT FOUND");
    // decide whether to alloc new page
    string line;
    vector<vector<Value>> data(table.record_num_per_page);
    int count = 0;
    while (true) {
        bool flag = false;
        int i = 0;
        for (; i < table.record_num_per_page; ++i) {
            if (!getline(file, line)) {
                flag = true;
                break;
            }
            // split line
            vector<string> record;
            stringstream ss(line);
            string token;
            while (getline(ss, token, ',')) {
                record.push_back(token);
            }
            data[i] = table.str_to_record(record);
            count++;
        }
        data.resize(i);
        table.write_whole_page(data);
        if (flag) break;
    }
    file.close();
    output_sys.output({{"rows"}, {to_string(count)}});
    return {};
}

std::any Visitor::visitSelect_table_(SQLParser::Select_table_Context *context) {
    return visitChildren(context);
}

std::any Visitor::visitSelect_table(SQLParser::Select_tableContext *context) {
    auto selectors = context->selectors()->selector();
    auto table_names = context->identifiers()->Identifier();
    for (auto &table_name: table_names) {
        auto table_index = current_db->get_table_index(table_name->getText());
        if (table_index == -1) {
            throw Error("TABLE DOESN'T EXIST");
        }
        auto &table = current_db->tables[table_index];
        int num = (int) table.record_num;
        vector<string> column_name;
        vector<int> selected_index;
        if (selectors.empty()) {
            vector<string> column_name;
            column_name.reserve(table.fields.size());
            for (auto &field: table.fields) {
                column_name.push_back(field.name);
            }
            output_sys.output({column_name});
        }else{
            for (auto &selector: selectors) {
                if (selector->column()) {
                    auto column = selector->column()->getText();
                    column_name.push_back(column);
                    selected_index.push_back(current_db->get_column_index({table.name, column}));
                } else {
                    throw UnimplementedError();
                }
            }
            output_sys.output({column_name});
        }
        int chunk_size = 1024;
        for (int i = 0; i < num; i += chunk_size) {
            auto content = table.get_record_range({i, min(i + chunk_size - 1, num - 1)});
            if (context->where_and_clause()) {
                auto where_and_clause = context->where_and_clause();
                auto where_clause = where_and_clause->where_clause();
                for (auto &clause: where_clause) {
                    // check type, if type is Where_operator_selectContext
                    // then it's a select statement
                    std::any result = clause->accept(this);
                    auto ptr = std::any_cast<std::shared_ptr<Where>>(result);
                    content.erase(std::remove_if(content.begin(), content.end(), [ptr](optional<Record> &record) {
                                      if (!record.has_value()) return false;
                                      return !ptr->choose(record.value());
                                  }),
                                  content.end());
                }
            }
            if (selectors.empty()) {
                output_sys.output(table.records_to_str(content));
            }else{
                auto output = table.records_to_str(content);
                vector<vector<string>> selected_content;
                for (auto &record: output) {
                    vector<string> selected_record;
                    selected_record.reserve(selected_index.size());
                    for (auto &index: selected_index) {
                        selected_record.push_back(record[index]);
                    }
                    selected_content.push_back(selected_record);
                }
                output_sys.output(selected_content);
            }
        }
    }
    return {};
}
std::any Visitor::visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *context) {
    std::shared_ptr<Where> where = std::make_shared<OperatorExpression>();
    auto operator_expression = std::dynamic_pointer_cast<OperatorExpression>(where);
    for (auto &column: context->column()->Identifier()) {
        operator_expression->column.push_back(column->getText());
    }
    operator_expression->op = context->operator_()->getText();
    if (auto value = context->expression()->value()) {
        if (value->String()) {
            operator_expression->value = value->getText().substr(1, value->getText().size() - 2);
        } else {
            operator_expression->value = value->getText();
        }
    }
    return where;
}
std::any Visitor::visitInsert_into_table(SQLParser::Insert_into_tableContext *context) {
    auto table_name = context->Identifier()->getText();
    int table_index = current_db->get_table_index(table_name);
    if (table_index == -1) {
        throw Error("TABLE DOESN'T EXIST");
    }
    auto &table = current_db->tables[table_index];
    auto value_lists = context->value_lists()->value_list();
    for (auto & value_list: value_lists){
        int n = value_list->value().size();
        if (n != table.fields.size()) {
            throw Error("VALUE NUMBER DOESN'T MATCH");
        }
        vector<Value> record;
        for (int i = 0; i < n; ++i) {
            auto value = value_list->value()[i];
            switch (table.fields[i].type) {
                case FieldType::INT:
                    record.emplace_back(std::stoi(value->getText()));
                    break;
                case FieldType::FLOAT:
                    record.emplace_back(std::stod(value->getText()));
                    break;
                case FieldType::VARCHAR:
                    record.emplace_back(value->getText().substr(1, value->getText().size() - 2));
                    break;
            }
        }
        table.add_record(record);
    }
    output_sys.output({{"rows"}, {to_string(value_lists.size())}});
    return {};
}
std::any Visitor::visitUpdate_table(SQLParser::Update_tableContext *context) {
    auto table_name = context->Identifier()->getText();
    int table_index = current_db->get_table_index(table_name);
    if (table_index == -1) {
        throw Error("TABLE DOESN'T EXIST");
    }
    auto &table = current_db->tables[table_index];
    auto set_clause = context->set_clause();
    int n = set_clause->Identifier().size();
    if (n != set_clause->value().size() || n!= set_clause->EqualOrAssign().size()) {
        throw Error("VALUE NUMBER DOESN'T MATCH");
    }
    auto where_clause = context->where_and_clause();
    vector<shared_ptr<Where>> checker;
    if (where_clause) {
        for (auto &clause: where_clause->where_clause()) {
            std::any result = clause->accept(this);
            auto ptr = std::any_cast<std::shared_ptr<Where>>(result);
            checker.push_back(ptr);
        }
    }
    int count = 0;
    for (int i = 0; i < table.record_num; ++i) {
        bool flag = true;
        auto record = table.get_record_range({i,i})[0];
        if (!record.has_value()) continue;
        for (auto &ptr: checker) {
            if(ptr->column.size() == 1){ ptr->column = {table.name, ptr->column[0]}; }
            if (!ptr->choose(record.value())) {
                flag = false;
                break;
            }
        }
        if (!flag) continue;
        count++;
        for (int j = 0; j < n; ++j) {
            auto column = set_clause->Identifier()[j]->getText();
            if (column == table.primary_key.name) {
                throw Error("PRIMARY KEY CAN'T BE UPDATED");
            }
            int index = current_db->get_column_index({table.name, column});
            if (index == -1) {
                throw Error("COLUMN DOESN'T EXIST");
            }
            auto value = set_clause->value()[j];
            switch (table.fields[index].type) {
                case FieldType::INT:
                    record.value()[index] = std::stoi(value->getText());
                    break;
                case FieldType::FLOAT:
                    record.value()[index] = std::stod(value->getText());
                    break;
                case FieldType::VARCHAR:
                    record.value()[index] = value->getText().substr(1, value->getText().size() - 2);
                    break;
            }
        }
        table.update_record(i, record.value());
    }
    output_sys.output({{"rows"}, {to_string(count)}});
    return {};
}
std::any Visitor::visitDelete_from_table(SQLParser::Delete_from_tableContext *context) {
    auto table_name = context->Identifier()->getText();
    int table_index = current_db->get_table_index(table_name);
    if (table_index == -1) {
        throw Error("TABLE DOESN'T EXIST");
    }
    auto &table = current_db->tables[table_index];
    auto where_clause = context->where_and_clause();
    vector<shared_ptr<Where>> checker;
    if (where_clause) {
        for (auto &clause: where_clause->where_clause()) {
            std::any result = clause->accept(this);
            auto ptr = std::any_cast<std::shared_ptr<Where>>(result);
            checker.push_back(ptr);
        }
    }
    int count = 0;
    for (int i = 0; i < table.record_num; ++i) {
        bool flag = true;
        auto record = table.get_record_range({i,i})[0];
        if (!record.has_value()) continue;
        for (auto &ptr: checker) {
            if(ptr->column.size() == 1){ ptr->column = {table.name, ptr->column[0]}; }
            if (!ptr->choose(record.value())) {
                flag = false;
                break;
            }
        }
        if (!flag) continue;
        count++;
        table.delete_record(i);
    }
    output_sys.output({{"rows"}, {to_string(count)}});
    return {};
}
