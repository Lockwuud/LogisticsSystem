#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include "Goods.h"

namespace Utils {
    // 字符串分割 (用于解析CSV)
    std::vector<std::string> split(const std::string &s, char delimiter);
    
    // 计算日期差 (返回 days)
    double daysFromToday(const std::string& dateStr);

    // 格式化输出浮点数
    std::string formatDouble(double value);
}

#endif // UTILS_H