#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

class DBManager {
public:
    // 创建数据表 (创建一个新的CSV文件)
    static bool createTable(const std::string& tableName, const std::vector<std::string>& headers);

    // 写入数据 (互斥锁 - 写者)
    static bool insertRecord(const std::string& tableName, const std::string& recordLine);

    // 查询数据 (共享锁 - 读者)
    // key: 关键字, columnIndex: -1表示全文搜索，>=0表示搜索特定列
    static std::string query(const std::string& tableName, const std::string& key, int columnIndex = -1);

    // 获取所有表名
    static std::vector<std::string> getAllTables();

private:
    // 辅助：获取文件路径
    static std::string getFilePath(const std::string& tableName);
};

#endif // DB_MANAGER_H