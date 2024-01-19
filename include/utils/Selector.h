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
#include <variant>

using Value = std::variant<int, double, std::string>;
typedef std::vector<Value> Record;

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
    std::vector<std::string> column;
    virtual bool choose(Record record) = 0;
    virtual ~Where() = default;
};

class OperatorExpression : public Where {
public:
    std::string op;
    std::string value;
    bool choose(Record record) override;
    ~OperatorExpression() override = default;
};


#endif//DBS_SELECTOR_H
