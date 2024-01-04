//
// Created by 胡 on 2024/1/4.
//

#include "dbsystem/database.h"
#include "filesystem/bufmanager/BufPageManager.h"
#include "filesystem/fileio/FileManager.h"
auto fm = std::make_unique<FileManager>();

std::vector<Database> databases = std::vector<Database>();
void Database::use_database() {
    for (const auto &entry: std::filesystem::directory_iterator("data/base/" + name)) {
        Table table;
        table.name = entry.path().filename().string();
        fm->openFile(entry.path().string().c_str(), table.fileID);
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
    tables.push_back(table);
}

void Database::drop_table(const string &table_name) {
    for (auto &table: tables) {
        if (table.name == table_name) {
            fm->closeFile(table.fileID);
            std::filesystem::remove("data/base/" + name + "/" + table_name);
            tables.erase(std::remove_if(tables.begin(), tables.end(), [&table_name](Table &table) {
                return table.name == table_name;
            }), tables.end());
            return;
        }
    }
    std::cout << "@TABLE DOESN'T EXIST" << std::endl;
}
