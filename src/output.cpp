//
// Created by 胡 on 2024/1/10.
//

#include "utils/output.h"
#include <iostream>
#include <string>


void Output::batch_output(const std::vector<std::vector<std::string>> &data) {
    for (auto &row: data) {
        std::string line;
        for (auto &col: row) {
            line += col + ",";
        }
        line.pop_back();
        std::cout << line << std::endl;
    }
}

void Output::interactive_output(const std::vector<std::vector<std::string>> &data) {
    for (auto &row: data) {
        for (auto &col: row) {
            std::cout << col << "\t";
        }
        std::cout << std::endl;
    }
}
