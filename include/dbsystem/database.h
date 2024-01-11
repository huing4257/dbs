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
#include <optional>

class Database;
extern Database *current_db;

enum class FieldType {
    INT = 0,
    FLOAT = 1,
    VARCHAR = 2
};

using Value = std::variant<int, float, std::string>;

struct StringVisitor{
    std::string operator()(int value) const {
        return std::to_string(value);
    }
    std::string operator()(float value) const {
        auto str = std::to_string(value);
        return str.substr(0, str.find('.') + 2);
    }
    std::string operator()(const std::string &value) const {
        return value;
    }
};

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
    void record_to_buf(const std::vector<Value> &record, unsigned int *buf) const;
    std::vector<Value> buf_to_record(const unsigned int *buf) const;
public:
    std::vector<int> primary_key_index;
    // using byte as unit, align to 32 byte
    int record_length = 0;
    int record_num_per_page = 0;
    int record_num = 0;
    // use bitmap to record which place is empty
    static const int PAGE_HEADER = 64;

    std::string name;
    int fileID = -1;
    std::vector<Field> fields;
    PrimaryKey primary_key;
    std::vector<ForeignKey> foreign_keys;
    void write_file() const;
    void read_file();
    bool construct();

    [[nodiscard]] std::vector<Value> get_record(int offset) const;
    bool add_record(const std::vector<Value> &record);

    [[nodiscard]] std::vector<Value> str_to_record(const std::vector<std::string> &line) const;
    [[nodiscard]] std::vector<std::string> record_to_str(const std::vector<Value> &record) const;

    void write_whole_page(std::vector<std::vector<Value>> &data);

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
    [[nodiscard]] int get_table_index(const std::string &table_name) const{
        for (int i = 0; i < tables.size(); i++) {
            if (tables[i].name == table_name) {
                return i;
            }
        }
        return -1;
    }

    ~Database() {
        close_database();
    }
};

extern std::vector<Database> databases;

void init_database();

#endif//DBS_DATABASE_H
