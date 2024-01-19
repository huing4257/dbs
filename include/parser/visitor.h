//
// Created by 胡 on 2023/12/30.
//

#ifndef DBS_VISITOR_H
#define DBS_VISITOR_H

#include "dbsystem/database.h"
#include "SQLVisitor.h"


class Visitor: public SQLVisitor {
    std::any visitProgram(SQLParser::ProgramContext *context) override;
    std::any visitStatement(SQLParser::StatementContext *context) override;
    std::any visitCreate_db(SQLParser::Create_dbContext *context) override;
    std::any visitDrop_db(SQLParser::Drop_dbContext *context) override;
    std::any visitShow_dbs(SQLParser::Show_dbsContext *context) override;
    std::any visitUse_db(SQLParser::Use_dbContext *context) override;
    std::any visitShow_tables(SQLParser::Show_tablesContext *context) override;
    std::any visitShow_indexes(SQLParser::Show_indexesContext *context) override { return {}; }
    std::any visitCreate_table(SQLParser::Create_tableContext *context) override;
    std::any visitDrop_table(SQLParser::Drop_tableContext *context) override;
    std::any visitDescribe_table(SQLParser::Describe_tableContext *context) override;
    std::any visitLoad_table(SQLParser::Load_tableContext *context) override;
    std::any visitInsert_into_table(SQLParser::Insert_into_tableContext *context) override;
    std::any visitDelete_from_table(SQLParser::Delete_from_tableContext *context) override;
    std::any visitUpdate_table(SQLParser::Update_tableContext *context) override;
    std::any visitSelect_table_(SQLParser::Select_table_Context *context) override;
    std::any visitSelect_table(SQLParser::Select_tableContext *context) override;
    std::any visitAlter_add_index(SQLParser::Alter_add_indexContext *context) override;
    std::any visitAlter_drop_index(SQLParser::Alter_drop_indexContext *context) override;
    std::any visitAlter_table_drop_pk(SQLParser::Alter_table_drop_pkContext *context) override;
    std::any visitAlter_table_drop_foreign_key(SQLParser::Alter_table_drop_foreign_keyContext *context) override { return {}; }
    std::any visitAlter_table_add_pk(SQLParser::Alter_table_add_pkContext *context) override;
    std::any visitAlter_table_add_foreign_key(SQLParser::Alter_table_add_foreign_keyContext *context) override { return {}; }
    std::any visitAlter_table_add_unique(SQLParser::Alter_table_add_uniqueContext *context) override { return {}; }
    std::any visitField_list(SQLParser::Field_listContext *context) override { return {}; }
    std::any visitNormal_field(SQLParser::Normal_fieldContext *context) override;
    std::any visitPrimary_key_field(SQLParser::Primary_key_fieldContext *context) override;
    std::any visitForeign_key_field(SQLParser::Foreign_key_fieldContext *context) override;
    std::any visitType_(SQLParser::Type_Context *context) override { return {}; }
    std::any visitOrder(SQLParser::OrderContext *context) override { return {}; }
    std::any visitValue_lists(SQLParser::Value_listsContext *context) override { return {}; }
    std::any visitValue_list(SQLParser::Value_listContext *context) override { return {}; }
    std::any visitValue(SQLParser::ValueContext *context) override { return {}; }
    std::any visitWhere_and_clause(SQLParser::Where_and_clauseContext *context) override { return {}; }
    std::any visitWhere_operator_expression(SQLParser::Where_operator_expressionContext *context) override;
    std::any visitWhere_operator_select(SQLParser::Where_operator_selectContext *context) override { return {}; }
    std::any visitWhere_null(SQLParser::Where_nullContext *context) override { return {}; }
    std::any visitWhere_in_list(SQLParser::Where_in_listContext *context) override { return {}; }
    std::any visitWhere_in_select(SQLParser::Where_in_selectContext *context) override { return {}; }
    std::any visitWhere_like_string(SQLParser::Where_like_stringContext *context) override { return {}; }
    std::any visitColumn(SQLParser::ColumnContext *context) override { return {}; }
    std::any visitExpression(SQLParser::ExpressionContext *context) override { return {}; }
    std::any visitSet_clause(SQLParser::Set_clauseContext *context) override { return {}; }
    std::any visitSelectors(SQLParser::SelectorsContext *context) override { return {}; }
    std::any visitSelector(SQLParser::SelectorContext *context) override { return {}; }
    std::any visitIdentifiers(SQLParser::IdentifiersContext *context) override { return {}; }
    std::any visitOperator_(SQLParser::Operator_Context *context) override { return {}; }
    std::any visitAggregator(SQLParser::AggregatorContext *context) override { return {}; }
};

#endif//DBS_VISITOR_H
