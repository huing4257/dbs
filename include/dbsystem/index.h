//
// Created by 胡 on 2024/1/18.
//

#ifndef DBS_INDEX_H
#define DBS_INDEX_H
#include "dbsystem/database.h"

/*
 * IndexNode for Btree
 * if not leaf, record is (key,child_page_id)
 * if is leaf, record is (key,record_id)
 */

typedef  std::vector<int> Key;
typedef std::pair<Key,int> IndexRecord;
class PageNode;

class Index{
public:
    // header
    // first page is metadata
    std::string name;
    std::vector<std::string> keys;
    int m;
    int key_num;
    int root_page_id;
    int page_num;
    // temp
    int fileID = -1;
    std::vector<int> key_i;
    void write_file() const;
    void read_file();
    PageNode page_to_node(int page_id);
    void records_to_buf(unsigned int* buf, const std::vector<IndexRecord>& record) const;

    int search_page_node(const Key& key);
    std::optional<int> search_record(const Key& key);
    void insert(const Key &key, int record_id);
    void remove(const Key& key);

    Index(std::string name, std::vector<std::string> keys);
    Index()= default;
};

class PageNode{
public:
    int page_id;
    Index& meta;
    // header
    bool is_leaf;
    unsigned int pred_page;
    unsigned int succ_page;
    unsigned int parent_page;
    unsigned int record_num;
    // max num is ((8192)/4 -5)/(1+key_num)
    // min is max/2
    void update() const;
    void insert(const Key& key, int id);
    void split();
    void pop_front();
    void pop_back();
    void push_back(IndexRecord record);
    void push_front(IndexRecord record);
    [[nodiscard]] std::vector<IndexRecord> to_vec() const;
    // only use when is_leaf == true
    [[nodiscard]] std::optional<int> search(const Key &key) const;
    bool borrow();
    // only use when is_leaf == true
    void remove(const Key& key);
    void merge();
    void update_record(int i, const Key& key) const;
    int get_index_in_parent();
};

#endif//DBS_INDEX_H
