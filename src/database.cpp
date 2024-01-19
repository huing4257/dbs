//
// Created by 胡 on 2024/1/4.
//

#include "dbsystem/database.h"
#include "dbsystem/index.h"
#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
#include "utils/error.h"
#include <algorithm>
#include <cstring>
#include <queue>
#include <utility>

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
        auto tname = tables.back().name;
        for (auto index: tables.back()._index) {
            auto is_open = fm->openFile(("data/base/" + name + "/" + tname + "." + index.name + ".index").c_str(),
                                        index.fileID);
            if (!is_open) throw Error("INDEX FILE OPEN FAILED");
            index.read_file();
            table.fill_index_ki(index);
        }
    }
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
    int page_index;
    unsigned int offset = 0;
    // buf using 4 bytes as a unit
    BufType buf = bpm->allocPage(fileID, 0, page_index, false);
    // name
    auto name_length = (unsigned int) (name.size());
    buf[0] = name_length;
    memcpy(buf + 1, name.c_str(), name_length);
    offset += (name_length + 3) / 4 + 1;
    // fields
    auto field_num = (unsigned int) (fields.size());
    buf[offset] = field_num;
    offset += 1;
    for (const auto &field: fields) {
        // name
        auto field_name_length = (unsigned int) (field.name.size());
        buf[offset] = field_name_length;
        memcpy(buf + offset + 1, field.name.c_str(), field_name_length);
        offset += (field_name_length + 3) / 4 + 1;
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
                    offset += sizeof(int) / 4;
                    break;
                }
                case FieldType::FLOAT: {
                    auto value = std::get<double>(field.default_value.value());
                    memcpy(buf + offset, &value, sizeof(double));
                    offset += sizeof(double) / 4;
                    break;
                }
                case FieldType::VARCHAR: {
                    auto value = std::get<std::string>(field.default_value.value());
                    auto value_length = (unsigned int) (value.size());
                    memcpy(buf + offset, &value_length, sizeof(unsigned int));
                    offset += sizeof(unsigned int);
                    memcpy(buf + offset, value.c_str(), value_length);
                    offset += (value_length + 3) / 4;
                    break;
                }
            }
        } else {
            buf[offset] = 0;
            offset += 1;
        }
    }
    // primary key
    buf[offset] = (unsigned int) (primary_key.keys.size());
    offset += 1;
    if (!primary_key.keys.empty()) {
        buf[offset] = primary_key.name.size();
        offset += 1;
        memcpy(buf + offset, primary_key.name.c_str(), primary_key.name.size());
        offset += (primary_key.name.size() + 3) / 4;
        for (const auto &key: primary_key.keys) {
            buf[offset] = key.size();
            offset += 1;
            memcpy(buf + offset, key.c_str(), key.size());
            offset += (key.size() + 3) / 4;
        }
    }
    buf[offset] = (unsigned int) (foreign_keys.size());
    offset += 1;
    for (const auto &foreign_key: foreign_keys) {
        buf[offset] = foreign_key.name.size();
        offset += 1;
        memcpy(buf + offset, foreign_key.name.c_str(), foreign_key.name.size());
        offset += (foreign_key.name.size() + 3) / 4;
        buf[offset] = foreign_key.table_name.size();
        offset += 1;
        memcpy(buf + offset, foreign_key.table_name.c_str(), foreign_key.table_name.size());
        offset += (foreign_key.table_name.size() + 3) / 4;
        buf[offset] = foreign_key.keys.size();
        offset += 1;
        for (const auto &key: foreign_key.keys) {
            buf[offset] = key.size();
            offset += 1;
            memcpy(buf + offset, key.c_str(), key.size());
            offset += (key.size() + 3) / 4;
        }
        for (const auto &key: foreign_key.ref_keys) {
            buf[offset] = key.size();
            offset += 1;
            memcpy(buf + offset, key.c_str(), key.size());
            offset += (key.size() + 3) / 4;
        }
    }
    // record num
    buf[offset] = 0;
    meta_offset = offset;
    offset += 1;
    // index
    buf[offset] = (unsigned int) (_index.size());
    offset += 1;
    // obtain name, construct later
    for (const auto &index: _index) {
        buf[offset] = index.name.size();
        offset += 1;
        memcpy(buf + offset, index.name.c_str(), index.name.size());
        offset += (index.name.size() + 3) / 4;
    }
    bpm->markDirty(page_index);
}

void Table::read_file() {
    int index;
    int offset = 0;
    // buf using 4 bytes as a unit
    BufType buf = bpm->getPage(fileID, 0, index);
    // name
    auto name_length = buf[0];
    name = std::string((char *) (buf + 1), name_length);
    offset += (name_length + 3) / 4 + 1;
    // fields
    auto field_num = buf[offset];
    offset += 1;
    for (int i = 0; i < field_num; ++i) {
        Field field;
        // name
        auto field_name_length = buf[offset];
        field.name = std::string((char *) (buf + offset + 1), field_name_length);
        offset += (field_name_length + 3) / 4 + 1;
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
                    offset += sizeof(int) / 4;
                    field.default_value = value;
                    break;
                }
                case FieldType::FLOAT: {
                    double value;
                    memcpy(&value, buf + offset, sizeof(double));
                    offset += sizeof(double) / 4;
                    field.default_value = value;
                    break;
                }
                case FieldType::VARCHAR: {
                    unsigned int value_length;
                    memcpy(&value_length, buf + offset, sizeof(unsigned int));
                    offset += sizeof(unsigned int);
                    field.default_value = std::string((char *) (buf + offset), value_length);
                    offset += (value_length + 3) / 4;
                    break;
                }
            }
        } else {
            offset += 1;
        }
        fields.push_back(field);
    }
    primary_key.keys.clear();
    auto key_num = buf[offset];
    offset += 1;
    auto key_name_length = buf[offset];
    if (key_name_length != 0) {
        primary_key.name = string((char *) (buf + offset), key_name_length);
    }
    for (int i = 0; i < key_num; ++i) {
        int value_length = (int) buf[offset];
        offset += 1;
        primary_key.keys.emplace_back((char *) (buf + offset), value_length);
        offset += (value_length + 3) / 4;
    }
    int foreign_key_num = buf[offset];
    offset += 1;
    for (int i = 0; i < foreign_key_num; ++i) {
        ForeignKey foreign_key;
        auto fk_name_length = buf[offset];
        offset += 1;
        foreign_key.name = string((char *) (buf + offset), fk_name_length);
        offset += (fk_name_length + 3) / 4;
        int table_name_length = buf[offset];
        offset += 1;
        foreign_key.table_name = string((char *) (buf + offset), table_name_length);
        offset += (table_name_length + 3) / 4;
        auto fk_key_num = buf[offset];
        offset += 1;
        for (int j = 0; j < fk_key_num; ++j) {
            int value_length = buf[offset];
            offset += 1;
            foreign_key.keys.emplace_back((char *) (buf + offset), value_length);
            offset += (value_length + 3) / 4;
        }
        for (int j = 0; j < fk_key_num; ++j) {
            int value_length = buf[offset];
            offset += 1;
            foreign_key.ref_keys.emplace_back((char *) (buf + offset), value_length);
            offset += (value_length + 3) / 4;
        }
        foreign_keys.push_back(foreign_key);
    }
    this->meta_offset = offset;
    record_num = buf[offset];
    offset += 1;
    // index
    int index_num = buf[offset];
    offset += 1;
    for (int i = 0; i < index_num; ++i) {
        Index tmp;
        auto index_name_length = buf[offset];
        offset += 1;
        tmp.name = string((char *) (buf + offset), index_name_length);
        offset += (index_name_length + 3) / 4;
        _index.push_back(tmp);
    }
}

void Table::update() const {
    int index;
    // buf using 4 bytes as a unit
    BufType buf = bpm->getPage(fileID, 0, index);
    buf[meta_offset] = record_num;
    bpm->markDirty(index);
}

bool Table::construct() {
    record_length = 4;
    for (auto &field: fields) {
        switch (field.type) {
            case FieldType::INT:
                record_length += sizeof(int);
                break;
            case FieldType::VARCHAR:
                record_length += (field.length + 1 + 3) / 4 * 4;
                break;
            case FieldType::FLOAT:
                record_length += sizeof(double);
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
        }
        int index = (int) (field - fields.begin());
        primary_key_index.push_back(index);
        fields[index].allow_null = false;
    }
    //todo: check foreign key
    return true;
}

void Table::add_index(const string &index_name, const vector<string> &keys) {
    _index.emplace_back(index_name, keys, false);
    fm->createFile(("data/base/" + current_db->name + "/" + name + "." + index_name + ".index").c_str());
    fm->openFile(("data/base/" + current_db->name + "/" + name + "." + index_name + ".index").c_str(), _index.back().fileID);
    _index.back().write_file();
    _index.back().init();
    write_file();
    fill_index_ki(_index.back());
    // todo: add index to index file
    unsigned int chunk_size = 1024;
    for (int i = 0; i < record_num; i += chunk_size) {
        auto content = get_record_range({i, min(i + chunk_size - 1, record_num - 1)});
        for (int j = 0; j < content.size(); ++j) {
            if (content[j]) {
                Key key;
                for (auto _i: _index.back().key_i) {
                    try {
                        int value = std::get<int>(content[j].value()[_i]);
                        key.push_back(value);
                    } catch (const std::bad_variant_access &e) {
                        throw Error("INDEX TYPE DOESN'T MATCH");
                    }
                }
                if (key[0] == 7) {
                    int a = 0;
                }
                _index.back().insert(key, i + j);
//                _index.back().draw_tree();
            }
        }
    }
}

void Table::record_to_buf(const vector<Value> &record, unsigned int *buf) const {
    int offset = 0;
    buf[0] = 1;
    offset += 1;
    for (int i = 0; i < fields.size(); ++i) {
        switch (fields[i].type) {
            case FieldType::INT: {
                auto value = std::get<int>(record[i]);
                memcpy(buf + offset, &value, sizeof(int));
                offset += sizeof(int) / 4;
                break;
            }
            case FieldType::FLOAT: {
                auto value = std::get<double>(record[i]);
                memcpy(buf + offset, &value, sizeof(double));
                offset += sizeof(double) / 4;
                break;
            }
            case FieldType::VARCHAR: {
                auto value = std::get<std::string>(record[i]);
                memcpy(buf + offset, value.c_str(), value.size());
                memcpy((unsigned char *) (buf + offset) + value.size(), "\0", 1);
                offset += (fields[i].length + 1 + 3) / 4;
                break;
            }
        }
    }
}

std::vector<Value> Table::buf_to_record(const unsigned int *buf) const {
    int offset = 1;
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
                double value;
                memcpy(&value, buf + offset, sizeof(double));
                offset += sizeof(double) / 4;
                record.emplace_back(value);
                break;
            }
            case FieldType::VARCHAR: {
                // use length to store varchar
                unsigned int value_length = field.length;
                string value((char *) (buf + offset));
                offset += (int) ((value_length + 1 + 3) / 4);
                record.emplace_back(value);
                break;
            }
        }
    }
    return record;
}

//todo: to be optimized
vector<optional<vector<Value>>> Table::get_record_range(std::pair<int, int> range) const {
    int index;
    int start = range.first;
    int end = range.second;
    if (start > end) {
        throw Error("START INDEX IS LARGER THAN END INDEX");
    }
    if (start < 0 || end >= record_num) {
        throw Error("INDEX OUT OF RANGE");
    }
    vector<optional<vector<Value>>> records;
    for (int i = start; i <= end; ++i) {
        int page_id = (i + 1) / record_num_per_page + 1;
        int offset = i % record_num_per_page;
        BufType buf = bpm->getPage(fileID, page_id, index);
        buf = buf + offset * record_length / 4 + PAGE_HEADER / 4;
        if (buf[0] == 0) {
            records.emplace_back(nullopt);
            continue;
        }
        records.emplace_back(buf_to_record(buf));
    }
    return records;
}

bool Table::add_record(const vector<Value> &record) {
    // todo: find deleted record
    int index;
    int page_id = (record_num + 1) / record_num_per_page + 1;
    int offset = (record_num) % record_num_per_page;
    BufType buf = bpm->getPage(fileID, page_id, index);
    buf = buf + offset * record_length / 4 + PAGE_HEADER / 4;
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
                auto f = std::get<double>(record[i]);
                string res;
                stringstream ss;
                ss << fixed << setprecision(2) << f;
                ss >> res;
                strs.emplace_back(res);
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
    for (auto &record: data) {
        record_to_buf(record, buf);
        buf += record_length / 4;
    }
    update();
    bpm->markDirty(index);
}

void Table::update_record(int i, const std::vector<Value> &record) {
    int index;
    int page_id = (i + 1) / record_num_per_page + 1;
    int offset = i % record_num_per_page;
    BufType buf = bpm->getPage(fileID, page_id, index);
    buf = buf + offset * record_length / 4 + PAGE_HEADER / 4;
    record_to_buf(record, buf);
    bpm->markDirty(index);
}

void Table::delete_record(int i) {
    int index;
    int page_id = (i + 1) / record_num_per_page + 1;
    int offset = i % record_num_per_page;
    BufType buf = bpm->getPage(fileID, page_id, index);
    buf = buf + offset * record_length / 4 + PAGE_HEADER / 4;
    buf[0] = 0;
    bpm->markDirty(index);
}

void PageNode::update() const {
    int index;
    BufType buf = bpm->getPage(meta.fileID, page_id, index);
    buf[0] = is_leaf;
    buf[1] = pred_page;
    buf[2] = succ_page;
    buf[3] = parent_page;
    buf[4] = record_num;
    bpm->markDirty(index);
}

// todo: to be optimized
optional<int> PageNode::search(const Key &key) const {
    int i = 0;
    auto records = to_vec();
    for (; i < records.size(); ++i) {
        if (records[i].first >= key) {
            return records[i].second;
        }
    }
    return nullopt;
}

void PageNode::insert(const Key &key, int id) {
    int index;
    BufType buf = bpm->getPage(meta.fileID, page_id, index);

    auto records = to_vec();
    if (records.empty() or key > records.back().first) {
        push_back({key, id});
        if (parent_page == 0) return;
        // update parent and ancestor
        auto parent_page_node = make_shared<PageNode>(meta.page_to_node((int) parent_page));
        parent_page_node->update_record(get_index_in_parent(), key);
        while (parent_page_node->parent_page != 0) {
            auto i = parent_page_node->get_index_in_parent();
            parent_page_node = make_shared<PageNode>(meta.page_to_node((int) parent_page_node->parent_page));
            parent_page_node->update_record(i, key);
            parent_page_node->update();
        }
        return;
    }
    auto it = std::lower_bound(records.begin(), records.end(),
                               key, [](const auto &record, const Key &k) {
                                   return record.first < k;
                               });
    records.insert(it, {key, id});
    record_num = static_cast<int>(records.size());
    buf[4] = record_num;
    int offset = 5;
    meta.records_to_buf((buf + offset), records);
    bpm->markDirty(index);
}

std::vector<IndexRecord> PageNode::to_vec() const {
    int index;
    BufType buf = bpm->getPage(meta.fileID, page_id, index);
    int offset = 5;
    std::vector<IndexRecord> records;
    for (int i = 0; i < record_num; ++i) {
        Key key;
        for (int j = 0; j < meta.key_num; ++j) {
            key.push_back((int) buf[offset + j]);
        }
        offset += meta.key_num;
        int record_id = (int) buf[offset];
        offset += 1;
        records.emplace_back(key, record_id);
    }
    bpm->access(index);
    return records;
}

void PageNode::pop_front() {
    record_num--;
    int index;
    BufType buf = bpm->getPage(meta.fileID, page_id, index);
    auto records = to_vec();
    records.erase(records.begin());
    buf[4] = record_num;
    int offset = 5;
    meta.records_to_buf((buf + offset), records);
    bpm->markDirty(index);
}

void PageNode::pop_back() {
    record_num--;
}

void PageNode::push_front(IndexRecord record) {
    record_num++;
    int index;
    auto buf = bpm->getPage(meta.fileID, page_id, index);
    buf[4] = record_num;
    int offset = 5;
    meta.records_to_buf((buf + offset), {std::move(record)});
    offset += meta.key_num + 1;
    auto records = to_vec();
    meta.records_to_buf((buf + offset), records);
    bpm->markDirty(index);
}

void PageNode::push_back(IndexRecord record) {
    int index;
    auto buf = bpm->getPage(meta.fileID, page_id, index);
    buf += 5 + record_num * (meta.key_num + 1);
    meta.records_to_buf(buf, {std::move(record)});
    record_num++;
    update();
}

void PageNode::split() {
    PageNode new_page = {
            (unsigned int) meta.page_num, meta, is_leaf, page_id, succ_page, parent_page, 0};
    meta.page_num += 1;
    // copy second half of records to new page
    auto records = to_vec();
    int left_num = (int) (records.size() / 2);
    for (int i = left_num; i < records.size(); ++i) {
        new_page.push_back({records[i].first, records[i].second});
        if (!is_leaf) {
            auto child_page = meta.page_to_node(records[i].second);
            child_page.parent_page = new_page.page_id;
            child_page.update();
        }
    }
    // update pred_page and succ_page
    if (succ_page != 0) {
        PageNode succ_page_node = meta.page_to_node((int) succ_page);
        succ_page_node.pred_page = new_page.page_id;
        succ_page_node.update();
    }
    succ_page = new_page.page_id;
    record_num = left_num;
    update();
    // update parent_page
    if (parent_page == 0) {
        // create new root
        PageNode new_root = {
                (unsigned int) meta.page_num, meta, false, 0, 0, 0, 0};
        new_root.update();
        meta.page_num++;
        new_root.insert(records[left_num - 1].first, page_id);
        new_root.insert(records[records.size() - 1].first, new_page.page_id);
        meta.root_page_id = new_root.page_id;
        new_page.parent_page = new_root.page_id;
        parent_page = new_root.page_id;
        new_root.update();
        new_page.update();
        update();
        return;
    }
    PageNode parent_page_node = meta.page_to_node((int) parent_page);
    parent_page_node.update_record(get_index_in_parent(), records[left_num - 1].first);
    parent_page_node.insert(records.back().first, new_page.page_id);
    parent_page_node.update();
    new_page.update();
}

bool PageNode::borrow() {
    if (pred_page == 0 && succ_page == 0) {
        return false;
    }
    if (pred_page != 0) {
        PageNode pred_page_node = meta.page_to_node((int) pred_page);
        if (pred_page_node.record_num >= (meta.m + 1) / 2) {
            // borrow from pred_page
            auto records = pred_page_node.to_vec();
            insert(records.back().first, records.back().second);
            records.pop_back();
            pred_page_node.record_num--;
            pred_page_node.update();
            auto index = pred_page_node.get_index_in_parent();
            auto parent_page_node = meta.page_to_node((int) parent_page);
            parent_page_node.update_record(index, records.back().first);
            return true;
        }
    }
    if (succ_page != 0) {
        PageNode succ_page_node = meta.page_to_node((int) succ_page);
        if (succ_page_node.record_num >= (meta.m + 1) / 2) {
            // borrow from succ_page
            auto records = succ_page_node.to_vec();
            insert(records.front().first, records.front().second);
            succ_page_node.pop_front();
            return true;
        }
    }
    return false;
}

void PageNode::update_record(int i, const Key &key) const {
    int index;
    auto buf = bpm->getPage(meta.fileID, page_id, index);
    buf = buf + 5 + i * (meta.key_num + 1);
    memcpy(buf, key.data(), sizeof(unsigned int) * meta.key_num);
    bpm->markDirty(index);
}

void PageNode::remove(const Key &key) {
    int i = 0;
    auto records = to_vec();
    for (; i < records.size(); ++i) {
        if (records[i].first == key) {
            break;
        }
    }
    if (i == records.size()) {
        throw Error("KEY DOESN'T EXIST");
    }
    records.erase(records.begin() + i);
    record_num--;
    int index;
    BufType buf = bpm->getPage(meta.fileID, page_id, index);
    buf[4] = record_num;
    meta.records_to_buf(buf + 5, records);
    bpm->markDirty(index);
}

void Index::records_to_buf(BufType buf, const std::vector<IndexRecord> &record) const {
    int offset = 0;
    for (const auto &r: record) {
        memcpy(buf + offset, r.first.data(), sizeof(unsigned int) * key_num);
        offset += key_num;
        memcpy(buf + offset, &r.second, sizeof(int));
        offset += 1;
    }
}

PageNode Index::page_to_node(unsigned int page_id) {
    int index;
    BufType buf = bpm->getPage(fileID, page_id, index);
    bool is_leaf = buf[0];
    unsigned int pred_page = buf[1];
    unsigned int succ_page = buf[2];
    unsigned int parent_page = buf[3];
    unsigned int record_num = buf[4];
    bpm->access(index);
    return {page_id, *this, is_leaf, pred_page, succ_page, parent_page, record_num};
}

int Index::search_page_node(const Key &key) {
    int page_id = root_page_id;
    while (true) {
        PageNode node = page_to_node(page_id);
        if (node.is_leaf) {
            return page_id;
        }
        int i = 0;
        auto records = node.to_vec();
        for (; i < records.size(); ++i) {
            if (records[i].first >= key) {
                break;
            }
        }
        if (i == records.size()) {
            page_id = records.back().second;
        } else {
            page_id = records[i].second;
        }
    }
}

optional<int> Index::search_record(const Key &key) {
    auto page_id = search_page_node(key);
    PageNode node = page_to_node(page_id);
    return node.search(key);
}

void Index::insert(const Key &key, int record_id) {
    auto page_id = search_page_node(key);
    auto page = make_shared<PageNode>(page_to_node(page_id));
    if (page->search(key) and is_unique) {
        throw Error("DUPLICATE KEY");
    }
    page->insert(key, record_id);
    while (page->record_num > m) {
        page->split();
        page = make_shared<PageNode>(page_to_node((int) page->parent_page));
    }
}

void Index::remove(const Key &key) {
    auto page_id = search_page_node(key);
    auto page = make_shared<PageNode>(page_to_node(page_id));
    page->remove(key);
    if (page->record_num >= m / 2) {
        return;
    }
    if (page->borrow()) {
        return;
    }
    // merge
    //    int child_index = page.
    //todo
}

int PageNode::get_index_in_parent() {
    int index = 0;
    auto parent_page_node = meta.page_to_node((int) parent_page);
    auto records = parent_page_node.to_vec();
    for (; index < records.size(); ++index) {
        if (records[index].second == page_id) {
            break;
        }
    }
    return index;
}

Index::Index(std::string _name, std::vector<std::string> _keys, bool _is_unique) {
    name = std::move(_name);
    keys = std::move(_keys);
    //    m = (PAGE_SIZE - 5) / (4 * (keys.size() + 1));
    //    m = (m/2)*2 - 1;
    //    if (m < 3) {
    //        throw Error("KEYS TOO LONG");
    //    }
    m = 3;
    key_num = keys.size();
    root_page_id = 1;
    page_num = 1;
    is_unique = _is_unique;
}

void Index::write_file() const {
    int index;
    // buf using 4 bytes as a unit
    BufType buf = bpm->allocPage(fileID, 0, index, false);
    // name
    auto name_length = (unsigned int) (name.size());
    buf[0] = name_length;
    memcpy(buf + 1, name.c_str(), name_length);
    int offset = (name_length + 3) / 4 + 1;
    // keys
    buf[offset] = key_num;
    offset += 1;
    for (const auto &key: keys) {
        auto key_length = (unsigned int) (key.size());
        buf[offset] = key_length;
        memcpy(buf + offset + 1, key.c_str(), key_length);
        offset += (key_length + 3) / 4 + 1;
    }
    // m
    buf[offset] = m;
    offset += 1;
    // key_num
    buf[offset] = key_num;
    offset += 1;
    // root_page_id
    buf[offset] = root_page_id;
    offset += 1;
    buf[offset] = is_unique;
    offset += 1;
    // page_num
    buf[offset] = page_num;
    offset += 1;
    bpm->markDirty(index);
}

void Index::read_file() {
    int index;
    int offset = 0;
    // buf using 4 bytes as a unit
    BufType buf = bpm->getPage(fileID, 0, index);
    // name
    auto name_length = buf[0];
    name = std::string((char *) (buf + 1), name_length);
    offset += (name_length + 3) / 4 + 1;
    // keys
    key_num = buf[offset];
    offset += 1;
    for (int i = 0; i < key_num; ++i) {
        auto key_length = buf[offset];
        offset += 1;
        keys.emplace_back((char *) (buf + offset), key_length);
        offset += (key_length + 3) / 4;
    }
    // m
    m = buf[offset];
    offset += 1;
    // key_num
    key_num = buf[offset];
    offset += 1;
    // root_page_id
    root_page_id = buf[offset];
    offset += 1;
    is_unique = buf[offset];
    offset += 1;
    // page_num
    page_num = buf[offset];
    offset += 1;
}

void Index::init() {
    page_num++;
    PageNode root = {1, *this, true, 0, 0, 0, 0};
    root.update();
}

void Index::draw_tree() {
    // to cerr
    cerr << "------- tree -------" << endl;
    std::queue<int> q;
    q.push(root_page_id);
    int level = 0;
    while (!q.empty()) {
        int nodes_in_current_level = q.size();
        cerr << "Level " << level << ": \n";// 输出层序号
        while (nodes_in_current_level > 0) {
            int page_id = q.front();
            q.pop();
            PageNode node = page_to_node(page_id);
            auto records = node.to_vec();

            cerr << "page_id: " << page_id << " is_leaf: " << node.is_leaf << " pred_page: " << node.pred_page
                 << " succ_page: " << node.succ_page << " parent_page: " << node.parent_page << " record_num: "
                 << node.record_num << endl;
            cerr << "records: ";
            for (const auto &record: records) {
                cerr << "(";
                for (const auto &key: record.first) {
                    cerr << key << " ";
                }
                cerr << ", " << record.second << ") ";
            }
            cerr << "\n";// 在记录输出后增加一些空格分隔
            if (!node.is_leaf) {
                for (const auto &record: records) {
                    q.push(record.second);
                }
            }
            --nodes_in_current_level;
        }
        cerr << endl;
        ++level;
    }
}

optional<vector<int>> Index::check(std::vector<std::shared_ptr<Where>> &checker) {
    vector<int> res;
    vector<shared_ptr<Where>> new_checker;
    bool flag = false;
    for (const auto &ptr: checker) {
        try {
            auto operator_checker = dynamic_pointer_cast<OperatorExpression>(ptr);
            if (operator_checker) {
                auto &key = operator_checker->column.back();
                // todo: multiple keys
                if (key == keys[0]) {
                    flag = true;
                    auto value = stoi(operator_checker->value);
                    auto page_id = search_page_node({value});
                    auto page = make_shared<PageNode>(page_to_node(page_id));
                    auto op = operator_checker->op;
                    if (op == "=" || op == ">" || op == ">=") {
                        // look forward
                        while (true) {
                            auto records = page->to_vec();
                            for (auto &record: records) {
                                if (op == "=" && record.first[0] == value) {
                                    res.push_back(record.second);
                                } else if (op == ">" && record.first[0] > value) {
                                    res.push_back(record.second);
                                } else if (op == ">=" && record.first[0] >= value) {
                                    res.push_back(record.second);
                                }
                            }
                            if (page->succ_page == 0) break;
                            page = make_shared<PageNode>(page_to_node((int) page->succ_page));
                        }
                    } else if (operator_checker->op == "<" || operator_checker->op == "<=") {
                        // look backward
                        do {
                            auto records = page->to_vec();
                            for (auto &record: records) {
                                if (op == "<" && record.first[0] < value) {
                                    res.push_back(record.second);
                                } else if (op == "<=" && record.first[0] <= value) {
                                    res.push_back(record.second);
                                }
                            }
                            page = make_shared<PageNode>(page_to_node((int) page->pred_page));
                        } while (page->pred_page != 0);
                    }
                } else {
                    new_checker.push_back(ptr);
                }
            }
        } catch (const std::bad_cast &e) {
            new_checker.push_back(ptr);
            continue;
        }
    }
    if (!flag) return nullopt;
    checker = new_checker;
    return res;
}