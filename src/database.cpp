//
// Created by 胡 on 2024/1/4.
//

#include "dbsystem/database.h"
#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
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
    for (const auto &entry: std::filesystem::directory_iterator("data/base/" + name)) {
        Table table;
        table.name = entry.path().filename().string();
        fm->openFile(entry.path().string().c_str(), table.fileID);
        table.read_file();
        tables.push_back(table);
    }
}

void Database::close_database() {
    for (auto &table: tables) {
        fm->closeFile(table.fileID);
    }
}
void Database::create_open_table(Table &table) {
    auto table_name = table.name;
    fm->createFile(("data/base/" + name + "/" + table_name).c_str());
    fm->openFile(("data/base/" + name + "/" + table_name).c_str(), table.fileID);
    table.write_file();
    tables.push_back(table);
}

void Database::drop_table(const string &table_name) {
    auto it = std::remove_if(tables.begin(), tables.end(), [&table_name](const Table &table) {
        return table.name == table_name;
    });

    if (it != tables.end()) {
        fm->closeFile(it->fileID);
        std::filesystem::remove("data/base/" + name + "/" + table_name);
        tables.erase(it, tables.end());
    } else {
        std::cout << "@TABLE DOESN'T EXIST" << std::endl;
    }
}
void Table::write_file() const {
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
    bpm->markDirty(index);
    bpm->writeBack(index);
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
}
bool Table::construct() {
    record_length = 0;
    for (auto &field: fields) {
        switch (field.type) {
            case FieldType::INT:
                record_length += sizeof(int);
                break;
            case FieldType::VARCHAR:
                record_length += field.length;
                break;
            case FieldType::FLOAT:
                record_length += sizeof(float);
                break;
        }
    }
    record_length = (record_length + 3) / 4 * 4;
    record_num_per_page = (PAGE_SIZE - 4) / record_length;
    for (auto &key: primary_key.keys) {
        auto field = std::find_if(fields.begin(), fields.end(), [&key](const Field &field) {
            return field.name == key;
        });
        if (field == fields.end()) {
            std::cout << "!ERROR " << key << " DOESN'T EXIST" << std::endl;
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
                offset += sizeof(int);
                break;
            }
            case FieldType::FLOAT: {
                auto value = std::get<float>(record[i]);
                memcpy(buf + offset, &value, sizeof(float));
                offset += sizeof(float);
                break;
            }
            case FieldType::VARCHAR: {
                auto value = std::get<std::string>(record[i]);
                memcpy(buf + offset, value.c_str(), value.size());
                offset += (int) value.size();
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
                offset += sizeof(int);
                record.emplace_back(value);
                break;
            }
            case FieldType::FLOAT: {
                float value;
                memcpy(&value, buf + offset, sizeof(float));
                offset += sizeof(float);
                record.emplace_back(value);
                break;
            }
            case FieldType::VARCHAR: {
                // use length to store varchar
                unsigned int value_length = field.length;
                string value((char *) (buf + offset), value_length);
                offset += (int) value_length;
                record.emplace_back(value);
                break;
            }
        }
    }
    return record;
}

std::vector<Value> Table::get_record(int offset) const {
    return std::vector<Value>();
}
