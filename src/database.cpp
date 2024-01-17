//
// Created by 胡 on 2024/1/4.
//

#include "dbsystem/database.h"
#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "utils/error.h"
#include <algorithm>
#include <cstring>

auto fm = std::make_unique<FileManager>();
auto bpm = std::make_unique<BufPageManager>(fm.get());

std::vector<Database> databases = std::vector<Database>();

void init_database() {
    for (const auto &entry: std::filesystem::directory_iterator("data/base")) {
        if (entry.is_directory()) {
            databases.emplace_back(entry.path().filename());
        }
    }
}
void Database::use_database() {
    MyBitMap::initConst();//新加的初始化
    if (!tables.empty()) {
        return;
    }
    for (const auto &entry: std::filesystem::directory_iterator("data/base/" + name)) {
        if (entry.path().extension().string() != ".db") continue;
        Table table;
        fm->openFile(entry.path().string().c_str(), table.fileID);
        table.read_file();
        table.construct();
        tables.push_back(table);
    }
    int a = 0;
}

void Database::close_database() {
    for (auto &table: tables) {
        table.update();
    }
    bpm->close();
    for (auto &table: tables) {
        fm->closeFile(table.fileID);
    }
}

void Database::create_open_table(Table table) {
    auto table_name = table.name;
    fm->createFile(("data/base/" + name + "/" + table_name + ".db").c_str());
    fm->openFile(("data/base/" + name + "/" + table_name + ".db").c_str(), table.fileID);
    tables.push_back(table);
    tables.back().write_file();
}

void Database::drop_table(const string &table_name) {
    auto it = std::remove_if(tables.begin(), tables.end(), [&table_name](const Table &table) {
        return table.name == table_name;
    });

    if (it != tables.end()) {
        fm->closeFile(it->fileID);
        std::filesystem::remove("data/base/" + name + "/" + table_name + ".db");
        tables.erase(it, tables.end());
    } else {
        throw Error("TABLE DOESN'T EXIST");
    }
}

void Table::write_file() {
    int index;
    unsigned int offset = 0;
    // buf using 4 bytes as a unit
    BufType buf = bpm->allocPage(fileID, 0, index, false);
    // name
    auto name_length = (unsigned int) (name.size());
    buf[0] = name_length;
    memcpy(buf + 1, name.c_str(), name_length);
    offset += name_length + 1;
    // fields
    auto field_num = (unsigned int) (fields.size());
    buf[offset] = field_num;
    offset += 1;
    for (const auto &field: fields) {
        // name
        auto field_name_length = (unsigned int) (field.name.size());
        buf[offset] = field_name_length;
        memcpy(buf + offset + 1, field.name.c_str(), field_name_length);
        offset += field_name_length + 1;
        // type
        buf[offset] = (unsigned int) (field.type);
        offset += 1;
        // length
        buf[offset] = field.length;
        offset += 1;
        // allow_null
        buf[offset] = field.allow_null;
        offset += 1;
        // default_value
        if (field.default_value) {
            buf[offset] = 1;
            offset += 1;
            switch (field.type) {
                case FieldType::INT: {
                    int value = std::get<int>(field.default_value.value());
                    memcpy(buf + offset, &value, sizeof(int));
                    offset += sizeof(int);
                    break;
                }
                case FieldType::FLOAT: {
                    float value = std::get<float>(field.default_value.value());
                    memcpy(buf + offset, &value, sizeof(float));
                    offset += sizeof(float);
                    break;
                }
                case FieldType::VARCHAR: {
                    auto value = std::get<std::string>(field.default_value.value());
                    auto value_length = (unsigned int) (value.size());
                    memcpy(buf + offset, &value_length, sizeof(unsigned int));
                    offset += sizeof(unsigned int);
                    memcpy(buf + offset, value.c_str(), value_length);
                    offset += value_length;
                    break;
                }
            }
        } else {
            buf[offset] = 0;
            offset += 1;
        }
    }
    buf[offset] = 0;
    meta_offset = offset;
    bpm->markDirty(index);
}

void Table::read_file() {
    int index;
    int offset = 0;
    // buf using 4 bytes as a unit
    BufType buf = bpm->getPage(fileID, 0, index);
    // name
    auto name_length = buf[0];
    name = std::string((char *) (buf + 1), name_length);
    offset += name_length + 1;
    // fields
    auto field_num = buf[offset];
    offset += 1;
    for (int i = 0; i < field_num; ++i) {
        Field field;
        // name
        auto field_name_length = buf[offset];
        field.name = std::string((char *) (buf + offset + 1), field_name_length);
        offset += field_name_length + 1;
        // type
        field.type = (FieldType) (buf[offset]);
        offset += 1;
        // length
        field.length = buf[offset];
        offset += 1;
        // allow_null
        field.allow_null = buf[offset];
        offset += 1;
        // default_value
        if (buf[offset]) {
            offset += 1;
            switch (field.type) {
                case FieldType::INT: {
                    int value;
                    memcpy(&value, buf + offset, sizeof(int));
                    offset += sizeof(int);
                    field.default_value = value;
                    break;
                }
                case FieldType::FLOAT: {
                    float value;
                    memcpy(&value, buf + offset, sizeof(float));
                    offset += sizeof(float);
                    field.default_value = value;
                    break;
                }
                case FieldType::VARCHAR: {
                    unsigned int value_length;
                    memcpy(&value_length, buf + offset, sizeof(unsigned int));
                    offset += sizeof(unsigned int);
                    field.default_value = std::string((char *) (buf + offset), value_length);
                    offset += value_length;
                    break;
                }
            }
        } else {
            offset += 1;
        }
        fields.push_back(field);
    }
    //todo: primary key
    this->meta_offset = offset;
    record_num = buf[offset];
}

void Table::update() const {
    int index;
    // buf using 4 bytes as a unit
    BufType buf = bpm->getPage(fileID, 0, index);
    buf[meta_offset] = record_num;
    bpm->markDirty(index);
}

bool Table::construct() {
    record_length = 0;
    for (auto &field: fields) {
        switch (field.type) {
            case FieldType::INT:
                record_length += sizeof(int);
                break;
            case FieldType::VARCHAR:
                record_length += (field.length + 3) / 4 * 4;
                break;
            case FieldType::FLOAT:
                record_length += sizeof(float);
                break;
        }
    }
    // align to 32 bytes
    record_length = (record_length + 31) / 32 * 32;
    record_num_per_page = (PAGE_SIZE - 64) / record_length;
    for (auto &key: primary_key.keys) {
        auto field = std::find_if(fields.begin(), fields.end(), [&key](const Field &field) {
            return field.name == key;
        });
        if (field == fields.end()) {
            throw Error("PRIMARY KEY DOESN'T EXIST");
            return false;
        }
        int index = (int) (field - fields.begin());
        primary_key_index.push_back(index);
        fields[index].allow_null = false;
    }
    //todo: check foreign key
    return true;
}

void Table::record_to_buf(const vector<Value> &record, unsigned int *buf) const {
    int offset = 0;
    for (int i = 0; i < fields.size(); ++i) {
        switch (fields[i].type) {
            case FieldType::INT: {
                auto value = std::get<int>(record[i]);
                memcpy(buf + offset, &value, sizeof(int));
                offset += sizeof(int) / 4;
                break;
            }
            case FieldType::FLOAT: {
                auto value = std::get<float>(record[i]);
                memcpy(buf + offset, &value, sizeof(float));
                offset += sizeof(float) / 4;
                break;
            }
            case FieldType::VARCHAR: {
                auto value = std::get<std::string>(record[i]);
                auto a = buf + offset;
                memcpy(buf + offset, value.c_str(), value.size());
                offset += (fields[i].length + 3) / 4;
                break;
            }
        }
    }
}

std::vector<Value> Table::buf_to_record(const unsigned int *buf) const {
    int offset = 0;
    std::vector<Value> record;
    for (const auto &field: fields) {
        switch (field.type) {
            case FieldType::INT: {
                int value;
                memcpy(&value, buf + offset, sizeof(int));
                offset += sizeof(int) / 4;
                record.emplace_back(value);
                break;
            }
            case FieldType::FLOAT: {
                float value;
                memcpy(&value, buf + offset, sizeof(float));
                offset += sizeof(float) / 4;
                record.emplace_back(value);
                break;
            }
            case FieldType::VARCHAR: {
                // use length to store varchar
                unsigned int value_length = field.length;
                string value((char *) (buf + offset), value_length);
                offset += (int) ((value_length + 3) / 4);
                record.emplace_back(value);
                break;
            }
        }
    }
    return record;
}

//todo: to be optimized
vector<vector<Value>> Table::get_record_range(std::pair<int,int> range) const {
    int index;
    int start = range.first;
    int end = range.second;
    if (start > end) {
        throw Error("START INDEX IS LARGER THAN END INDEX");
    }
    if (start < 0 || end >= record_num) {
        throw Error("INDEX OUT OF RANGE");
    }
    vector<vector<Value>> records;
    for (int i = start; i <= end; ++i) {
        int page_id = (i + 1) / record_num_per_page + 1;
        int offset = i % record_num_per_page;
        BufType buf = bpm->getPage(fileID, page_id, index);
        buf = buf + offset * record_length + PAGE_HEADER;
        records.push_back(buf_to_record(buf));
    }
    return records;
}

bool Table::add_record(const vector<Value> &record) {
    //    if (record.size() != fields.size()) {
    //        std::cout << "!ERROR RECORD SIZE DOESN'T MATCH";
    //        return false;
    //    }
    //    for (int i = 0; i < fields.size(); ++i) {
    //        if (record[i].index() != (int)fields[i].type) {
    //            std::cout << "!ERROR RECORD TYPE DOESN'T MATCH";
    //            return false;
    //        }
    //    }
    int index;
    int page_id = (record_num + 1) / record_num_per_page + 1;
    int offset = (record_num) % record_num_per_page;
    BufType buf = bpm->getPage(fileID, page_id, index);
    buf = buf + offset * record_length + PAGE_HEADER;
    record_to_buf(record, buf);
    bpm->markDirty(index);
    record_num++;
    return true;
}

vector<Value> Table::str_to_record(const std::vector<std::string> &strs) const {
    std::vector<Value> record;
    for (int i = 0; i < fields.size(); ++i) {
        switch (fields[i].type) {
            case FieldType::INT: {
                record.emplace_back(std::stoi(strs[i]));
                break;
            }
            case FieldType::FLOAT: {
                record.emplace_back(std::stof(strs[i]));
                break;
            }
            case FieldType::VARCHAR: {
                record.emplace_back(strs[i]);
                break;
            }
        }
    }
    return record;
}

vector<string> Table::record_to_str(const vector<Value> &record) const {
    std::vector<std::string> strs;
    for (int i = 0; i < fields.size(); ++i) {
        switch (fields[i].type) {
            case FieldType::INT: {
                strs.emplace_back(std::to_string(std::get<int>(record[i])));
                break;
            }
            case FieldType::FLOAT: {
                strs.emplace_back(std::to_string(std::get<float>(record[i])));
                break;
            }
            case FieldType::VARCHAR: {
                strs.emplace_back(std::get<std::string>(record[i]));
                break;
            }
        }
    }
    return strs;
}

// fast load, write whole new page
void Table::write_whole_page(vector<std::vector<Value>> &data) {
    int index;
    record_num += (int) data.size();
    int page_id = (record_num + 1) / record_num_per_page + 1;
    BufType buf = bpm->allocPage(fileID, page_id, index, false);
    buf += PAGE_HEADER / 4;
    int i = 0;
    for (auto &record: data) {
        i++;
        record_to_buf(record, buf);
        buf += record_length / 4;
    }
    update();
    bpm->markDirty(index);
}
