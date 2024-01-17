//
// Created by 胡 on 2024/1/18.
//

#ifndef DBS_SELECTOR_H
#define DBS_SELECTOR_H
/*
operator_expression
operator_select
null
in_list
in_select
like_string
 */
#include <string>
#include <vector>
#include "dbsystem/database.h"

enum class SelectorType {
    OPERATOR_EXPRESSION,
    OPERATOR_SELECT,
    NULL_,
    IN_LIST,
    IN_SELECT,
    LIKE_STRING
};

class Where {
public:
    virtual bool choose(Record record) = 0;
};

class OperatorExpression : public Where {
public:
    std::vector<std::string> column;
    std::string op;
    std::string value;
    bool choose(Record record) override;
};


#endif//DBS_SELECTOR_H
