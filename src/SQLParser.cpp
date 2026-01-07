#include "SQLParser.h"
#include "DBManager.h"
#include "Utils.h"
#include <sstream>
#include <algorithm>
#include <iostream>

// 辅助：转小写
std::string SQLParser::toLower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

// 1. 词法分析器 (Lexer)
std::vector<Token> SQLParser::tokenize(const std::string& sql) {
    std::vector<Token> tokens;
    std::string current;
    bool inString = false;
    
    for (size_t i = 0; i < sql.length(); ++i) {
        char c = sql[i];
        
        if (inString) {
            if (c == '\'') { // 字符串结束
                inString = false;
                tokens.push_back({current, "STRING"});
                current = "";
            } else {
                current += c;
            }
            continue;
        }

        if (isspace(c)) {
            if (!current.empty()) {
                tokens.push_back({toLower(current), "ID"}); // 暂定为 ID，后续判断 keyword
                current = "";
            }
        } else if (c == ',' || c == '(' || c == ')' || c == '=' || c == '*' || c == ';') {
            if (!current.empty()) {
                tokens.push_back({toLower(current), "ID"});
                current = "";
            }
            tokens.push_back({std::string(1, c), "SYMBOL"});
        } else if (c == '\'') { // 字符串开始
            inString = true;
        } else {
            current += c;
        }
    }
    if (!current.empty()) tokens.push_back({toLower(current), "ID"});

    // 修正关键字类型
    for (auto& t : tokens) {
        if (t.type == "ID") {
            if (t.value == "select" || t.value == "from" || t.value == "where" ||
                t.value == "insert" || t.value == "into" || t.value == "values" ||
                t.value == "create" || t.value == "table" ||
                t.value == "update" || t.value == "set" || t.value == "delete") { // <--- 新增
                t.type = "KEYWORD";
            }
        }
    }
    return tokens;
}

// 主执行函数
std::string SQLParser::execute(const std::string& sqlRaw) {
    // 简单处理：去掉末尾分号
    std::string sql = sqlRaw;
    if (!sql.empty() && sql.back() == ';') sql.pop_back();

    auto tokens = tokenize(sql);
    if (tokens.empty()) return "{\"success\":false, \"msg\":\"Empty SQL\"}";

    std::string cmd = tokens[0].value;
    
    if (cmd == "select") return parseSelect(tokens);
    if (cmd == "insert") return parseInsert(tokens);
    if (cmd == "create") return parseCreate(tokens);
    if (cmd == "update") return parseUpdate(tokens); 
    if (cmd == "delete") return parseDelete(tokens); 
    
    return "{\"success\":false, \"msg\":\"Unknown command: " + cmd + "\"}";
}

// --- 语法分析与对接 ---

// 解析: SELECT col FROM table [WHERE col=val]
std::string SQLParser::parseSelect(const std::vector<Token>& tokens) {
    // 简单模式匹配
    // tokens[1] 应该是列名 或 *
    // tokens[2] 应该是 FROM
    // tokens[3] 应该是 表名
    
    if (tokens.size() < 4 || tokens[2].value != "from") {
        return "{\"success\":false, \"msg\":\"Syntax Error: Expected SELECT ... FROM ...\"}";
    }

    std::string targetCol = tokens[1].value; // "*" 或 列名
    std::string tableName = tokens[3].value;
    
    // 处理 WHERE
    std::string whereKey = "";
    int queryColIdx = -1; // 默认全文搜索(-1) 或者 特定列搜索

    // 简单支持: WHERE col = 'val' 或 WHERE 'val'
    if (tokens.size() >= 6 && tokens[4].value == "where") {
        if (tokens.size() == 8 && tokens[6].value == "=") {
             // 格式: WHERE col = val
             std::string whereCol = tokens[5].value;
             whereKey = tokens[7].value;
             // 获取 WHERE 条件列的索引
             queryColIdx = DBManager::getColumnIndex(tableName, whereCol);
        } else if (tokens.size() == 6) {
             // 格式: WHERE val (简易模糊搜)
             whereKey = tokens[5].value;
        }
    }

    // 对接实验二：执行查询
    // 注意：实验二的 query 接口是 (tableName, key, searchColIdx)
    // 这里我们要返回所有数据，但只显示用户 SELECT 的列？
    // 为了简化，我们先利用 DBManager 获取所有符合条件的行
    
    std::string jsonRes = DBManager::query(tableName, whereKey, queryColIdx);
    
    // 如果用户只 SELECT 了特定列 (不是 *)，我们可以在这里对 jsonRes 再做一次过滤
    // 但为了不让代码过于复杂，本实验暂且直接返回整行数据，或者你可以修改 DBManager 支持 SELECT投影
    // 这里我们认为: 只要能查出数据就算成功。
    
    return "{\"success\":true, \"data\":" + jsonRes + "}";
}

// 解析: INSERT INTO table VALUES (v1, v2, ...)
std::string SQLParser::parseInsert(const std::vector<Token>& tokens) {
    // 0:INSERT 1:INTO 2:table 3:VALUES 4:( 5:val ...
    if (tokens.size() < 6 || tokens[1].value != "into" || tokens[3].value != "values") {
         return "{\"success\":false, \"msg\":\"Syntax Error: Expected INSERT INTO ... VALUES ...\"}";
    }

    std::string tableName = tokens[2].value;
    std::string dataLine = "";
    
    // 提取括号内的值
    for (size_t i = 5; i < tokens.size(); ++i) {
        if (tokens[i].value == ")") break;
        if (tokens[i].value == ",") {
            dataLine += ",";
        } else {
            dataLine += tokens[i].value; // 拼接值
        }
    }

    // 对接实验二：写入
    bool ok = DBManager::insertRecord(tableName, dataLine);
    return ok ? "{\"success\":true, \"msg\":\"Inserted successfully\"}" 
              : "{\"success\":false, \"msg\":\"Insert failed (Lock or IO error)\"}";
}

// 解析: CREATE TABLE table (col1, col2, ...)
std::string SQLParser::parseCreate(const std::vector<Token>& tokens) {
    // 0:CREATE 1:TABLE 2:name 3:( 4:col ...
    if (tokens.size() < 5 || tokens[1].value != "table" || tokens[3].value != "(") {
        return "{\"success\":false, \"msg\":\"Syntax Error: Expected CREATE TABLE name (...)\"}";
    }

    std::string tableName = tokens[2].value;
    std::vector<std::string> headers;
    
    for (size_t i = 4; i < tokens.size(); ++i) {
        if (tokens[i].value == ")") break;
        if (tokens[i].value != ",") {
            headers.push_back(tokens[i].value);
        }
    }

    // 对接实验二：建表
    bool ok = DBManager::createTable(tableName, headers);
    return ok ? "{\"success\":true, \"msg\":\"Table created\"}" 
              : "{\"success\":false, \"msg\":\"Table already exists\"}";
}

// 解析: UPDATE table SET col = val WHERE col = val
std::string SQLParser::parseUpdate(const std::vector<Token>& tokens) {
    // 0:UPDATE 1:table 2:SET 3:col 4:= 5:val 6:WHERE 7:col 8:= 9:val
    if (tokens.size() < 6 || tokens[2].value != "set" || tokens[4].value != "=") {
        return "{\"success\":false, \"msg\":\"Syntax Error: Expected UPDATE table SET col=val ...\"}";
    }

    std::string tableName = tokens[1].value;
    std::string targetCol = tokens[3].value;
    std::string newVal = tokens[5].value;
    
    std::string whereCol = "";
    std::string whereVal = "";

    if (tokens.size() >= 10 && tokens[6].value == "where" && tokens[8].value == "=") {
        whereCol = tokens[7].value;
        whereVal = tokens[9].value;
    }

    bool ok = DBManager::updateRecords(tableName, whereCol, whereVal, targetCol, newVal);
    return ok ? "{\"success\":true, \"msg\":\"Update success\"}" : "{\"success\":false, \"msg\":\"Update failed\"}";
}

// 解析: DELETE FROM table WHERE col = val
std::string SQLParser::parseDelete(const std::vector<Token>& tokens) {
    // 0:DELETE 1:FROM 2:table 3:WHERE 4:col 5:= 6:val
    if (tokens.size() < 3 || tokens[1].value != "from") {
        return "{\"success\":false, \"msg\":\"Syntax Error: Expected DELETE FROM table ...\"}";
    }

    std::string tableName = tokens[2].value;
    std::string whereCol = "";
    std::string whereVal = "";

    if (tokens.size() >= 7 && tokens[3].value == "where" && tokens[5].value == "=") {
        whereCol = tokens[4].value;
        whereVal = tokens[6].value;
    } else if (tokens.size() == 3) {
        // 没有 WHERE，那就是清空表？暂且允许
    } else {
        return "{\"success\":false, \"msg\":\"Syntax Error: Invalid WHERE clause\"}";
    }

    bool ok = DBManager::deleteRecords(tableName, whereCol, whereVal);
    return ok ? "{\"success\":true, \"msg\":\"Delete success\"}" : "{\"success\":false, \"msg\":\"Delete failed\"}";
}