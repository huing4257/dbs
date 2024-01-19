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
#include <sstream>
#include "utils/error.h"
#include "dbsystem/index.h"

// -------db-------

class Database;
extern Database *current_db;

enum class FieldType {
    INT = 0,
    FLOAT = 1,
    VARCHAR = 2
};

using Value = std::variant<int, double, std::string>;

typedef std::vector<Value> Record;

struct StringVisitor{
    std::string operator()(int value) const {
        return std::to_string(value);
    }
    std::string operator()(double value) const {
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
    int fileID = -1;
    std::vector<int> primary_key_index;
    // using byte as unit, align to 32 byte
    int record_length = 0;
    int record_num_per_page = 0;
    unsigned int record_num = 0;
    // use bitmap to record which place is empty
    static const int PAGE_HEADER = 64;
    // end of metadata
    unsigned int meta_offset = 0;
    std::string name;
    std::vector<Field> fields;
    PrimaryKey primary_key;
    std::vector<ForeignKey> foreign_keys;
    std::vector<Index> _index;
    void write_file();
    void read_file();
    bool construct();
    void update() const;
    void add_index(const std::string& index_name, const std::vector<std::string>& keys);
    void fill_index_ki(Index &i){
        i.key_i.clear();
        for (auto &key : i.keys) {
            for (int j = 0; j < fields.size(); j++) {
                if (fields[j].name == key) {
                    i.key_i.push_back(j);
                    break;
                }
            }
        }
    }

    bool add_record(const std::vector<Value> &record);
    [[nodiscard]] std::vector<Value> str_to_record(const std::vector<std::string> &line) const;
    [[nodiscard]] std::vector<std::string> record_to_str(const std::vector<Value> &record) const;
    [[nodiscard]] std::vector<std::vector<std::string>> records_to_str(const std::vector<std::optional<std::vector<Value>>> &record) const {
        std::vector<std::vector<std::string>> res;
        for (auto &r : record) {
            if (r) {
                res.push_back(record_to_str(r.value()));
            }
        }
        return res;
    }
    [[nodiscard]] std::vector<std::optional<std::vector<Value>>> get_record_range(std::pair<int,int>) const;

    [[nodiscard]] std::vector<std::optional<std::vector<Value>>> all_records() const{
        return get_record_range({0, record_num-1});
    }

    void update_record(int i, const std::vector<Value>& record);
    void write_whole_page(std::vector<std::vector<Value>> &data);
    void delete_record(int i);

    Table() = default;
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
    void drop_table(const std::string &table_name);
    [[nodiscard]] int get_table_index(const std::string &table_name) const{
        for (int i = 0; i < tables.size(); i++) {
            if (tables[i].name == table_name) {
                return i;
            }
        }
        return -1;
    }

    void create_open_table(Table table);

    [[nodiscard]] int get_column_index(const std::vector<std::string> &column) const {
        for (auto &t : tables) {
            if (t.name == column[0]) {
                for (int i = 0; i < t.fields.size(); i++) {
                    if (t.fields[i].name == column[1]) {
                        return i;
                    }
                }
            }
        }
        throw Error("No such column");
    }

    ~Database() {
        close_database();
    }
};

extern std::vector<Database> databases;

void init_database();

#endif//DBS_DATABASE_H
