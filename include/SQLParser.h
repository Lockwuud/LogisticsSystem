#ifndef SQL_PARSER_H
#define SQL_PARSER_H

#include <string>
#include <vector>

// Token 结构定义
struct Token {
    std::string value;
    std::string type; // "KEYWORD", "ID", "STRING", "SYMBOL"
};

class SQLParser {
public:
    // 对外唯一接口：执行 SQL 语句并返回 JSON 结果
    static std::string execute(const std::string& sql);

private:
    // 1. 词法分析：字符串 -> Token 列表
    static std::vector<Token> tokenize(const std::string& sql);

    // 2. 语法分析与执行
    static std::string parseSelect(const std::vector<Token>& tokens);
    static std::string parseInsert(const std::vector<Token>& tokens);
    static std::string parseCreate(const std::vector<Token>& tokens);
    static std::string parseUpdate(const std::vector<Token>& tokens);
    static std::string parseDelete(const std::vector<Token>& tokens);
    
    // 辅助函数
    static std::string toLower(const std::string& str);
};

#endif