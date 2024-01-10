//
// Created by 胡 on 2024/1/4.
//

#ifndef DBS_DATABASE_H
#define DBS_DATABASE_H
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include <variant>

class Database;
extern Database *current_db;

enum class FieldType {
    INT,
    VARCHAR,
    FLOAT
};

using Value = std::variant<int, float, std::string>;

class Field {
public:
    std::string name;
    FieldType type;
    int length = 0;
    bool allow_null = true;
    std::optional<Value> default_value;
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
    std::vector<int> primary_key_index;
    int record_length = 0;
    int record_num_per_page = 0;
    int record_num = 0;
    // use bitmap to record which place is empty
    static const int PAGE_HEADER = 64;

    void record_to_buf(const std::vector<Value> &record, unsigned int *buf) const;
    std::vector<Value> buf_to_record(const unsigned int *buf) const;
public:
    std::string name;
    int fileID = -1;
    std::vector<Field> fields;
    PrimaryKey primary_key;
    std::vector<ForeignKey> foreign_keys;
    void write_file() const;
    void read_file();
    bool construct();
    std::vector<Value> get_record(int offset) const;
    bool add_record(const std::vector<Value> &record);
    Table() = default;
    Table(const Table &table) {
        name = table.name;
        fileID = table.fileID;
        fields = table.fields;
        primary_key = table.primary_key;
        foreign_keys = table.foreign_keys;
        primary_key_index = table.primary_key_index;
        record_length = table.record_length;
        record_num_per_page = table.record_num_per_page;
        record_num = table.record_num;
    }
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
