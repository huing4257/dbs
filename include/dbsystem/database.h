//
// Created by 胡 on 2024/1/4.
//

#ifndef DBS_DATABASE_H
#define DBS_DATABASE_H
#include <any>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

class Database;
extern Database *current_db;

enum class FieldType {
    INT,
    VARCHAR,
    FLOAT
};

class Field {
public:
    std::string name;
    FieldType type;
    int length = 0;
    bool allow_null = true;
    std::any default_value;
};

class PrimaryKey {
public:
    std::string name;
    std::vector<std::string> keys;
};

class ForeignKey {
public:
    std::string name;
    std::vector<std::string> keys;
    std::string table_name;
    std::vector<std::string> ref_keys;
};

class Table {
private:
    void record_to_buf(const std::vector<std::any> &record, unsigned int *buf) const;
    std::vector<std::any> buf_to_record(const unsigned int *buf) const;
    std::vector<int> primary_key_index;
    int record_length = 0;
public:
    std::string name;
    int fileID = -1;
    std::vector<Field> fields;
    PrimaryKey primary_key;
    std::vector<ForeignKey> foreign_keys;
    void write_file() const;
    void read_file();
    bool construct();
    std::vector<std::any> get_record(int offset) const;
    bool add_record(const std::vector<std::any> &record);
};

class Database {
public:
    std::string name;
    std::vector<Table> tables;
    explicit Database(std::string name) : name(std::move(name)) {
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

void init_database();

#endif//DBS_DATABASE_H
