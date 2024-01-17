//
// Created by 胡 on 2024/1/18.
//

#include "utils/Selector.h"
#include "dbsystem/database.h"

bool OperatorExpression::choose(Record record) {
    if (op == "=") {
        return std::visit(StringVisitor{}, record[current_db->get_column_index(column)]) == value;
    } else if (op == "<>") {
        return std::visit(StringVisitor{}, record[current_db->get_column_index(column)]) != value;
    } else {
        auto record_value = record[current_db->get_column_index(column)];
        if (record_value.index() == 0){
            int record_int = std::get<int>(record_value);
            int value_int = std::stoi(value);
            if (op == ">") {
                return record_int > value_int;
            } else if (op == "<") {
                return record_int < value_int;
            } else if (op == ">=") {
                return record_int >= value_int;
            } else if (op == "<=") {
                return record_int <= value_int;
            } else {
                throw Error("Unknown operator: " + op);
            }
        }
        else if (record_value.index() == 1){
            float record_float = std::get<float>(record_value);
            float value_float = std::stof(value);
            if (op == ">") {
                return record_float > value_float;
            } else if (op == "<") {
                return record_float < value_float;
            } else if (op == ">=") {
                return record_float >= value_float;
            } else if (op == "<=") {
                return record_float <= value_float;
            } else {
                throw Error("Unknown operator: " + op);
            }
        }
        else {
            throw Error("Unknown operator: " + op);
        }
    }
}
