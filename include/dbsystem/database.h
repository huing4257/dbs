//
// Created by 胡 on 2024/1/4.
//

#ifndef DBS_DATABASE_H
#define DBS_DATABASE_H
#include <string>
#include <utility>
#include <vector>
#include <filesystem>

enum class FieldType {
    INT,
    VARCHAR,
    FLOAT
};

class Field {
public:
    // TODO: add default value
    std::string name;
    FieldType type;
    int length = 0;
    bool is_primary_key = false;
    bool is_foreign_key = false;
//    std::string foreign_key_table;
//    std::string foreign_key_field;
};

class Table {
public:
    std::string name;
    int fileID = -1;
    std::vector<Field> fields;
};

class Database {
public:
    std::string name;
    std::vector<Table> tables;
    explicit Database(std::string name): name(std::move(name)) {
        tables = std::vector<Table>();
    }
    void use_database();
    void close_database();
    void create_open_table(Table &table);
    void drop_table(const std::string &table_name);
    ~Database() {
        close_database();
    }
};

extern std::vector<Database> databases;
#endif//DBS_DATABASE_H
