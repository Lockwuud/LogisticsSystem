#include <iostream>
#include "GlobalData.h"
#include "WebGui.h"
#include "Utils.h"
#include "Goods.h"

SchemeNode logisticsTree[5]; 
std::vector<std::string> belongingRegionList;

int main() {
    #ifdef _WIN32
    system("chcp 65001");
    #endif

    // 初始化方案树结构
    logisticsTree[1] = {1, "价格最小方案", {}};
    logisticsTree[2] = {2, "时间最短方案", {}};
    logisticsTree[3] = {3, "综合最优方案", {}};
    logisticsTree[4] = {4, "航空物流方案", {}};

    WebGui gui;
    gui.init(); // 加载数据
    gui.start(); // 启动服务

    return 0;
}