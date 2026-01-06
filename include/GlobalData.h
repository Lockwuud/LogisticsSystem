#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#include <vector>
#include <string>
#include "Goods.h"

// 将 SchemeNode 的定义移到这里，这样 Main.cpp 和 WebGui.cpp 都能看到它
struct SchemeNode {
    int schemeId;           // 1:价格最小, 2:时间最短, 3:综合最优, 4:航空
    std::string schemeName;
    std::vector<Goods> goodsList; // 该方案下的货物
};

// 声明全局变量（实际定义还在 Main.cpp 中）
extern SchemeNode logisticsTree[5];
extern std::vector<std::string> belongingRegionList;

#endif // GLOBAL_DATA_H