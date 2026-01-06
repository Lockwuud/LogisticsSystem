#include <iostream>
#include <vector>
#include <string>
#include <fstream>  
#include <set>       
#include <algorithm> 

#include "GlobalData.h"
#include "WebGui.h"
#include "Utils.h"
#include "Goods.h"

// 定义全局变量
SchemeNode logisticsTree[5]; 
std::vector<std::string> belongingRegionList;

// 前向声明
void initData(const std::string& filename); 

int main() {
    // 设置控制台编码，防止乱码
    #ifdef _WIN32
    system("chcp 65001");
    #endif

    // 初始化方案树的名称
    logisticsTree[1] = {1, "价格最小方案", {}};
    logisticsTree[2] = {2, "时间最短方案", {}};
    logisticsTree[3] = {3, "综合最优方案", {}};
    logisticsTree[4] = {4, "航空物流方案", {}};

    std::cout << "------------欢迎来到物流管理系统(Web版)-----------" << std::endl;
    std::cout << "正在初始化系统数据..." << std::endl;

    // 初始化数据
    initData("../data/goodsInfo.csv"); 

    // 启动 Web GUI
    WebGui gui;
    gui.start();

    return 0;
}

void initData(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "警告: 无法打开数据文件: " << filename << "，系统将以空数据启动。" << std::endl;
        return;
    }

    std::string line;
    std::set<std::string> regionSet;
    
    // 跳过表头
    std::getline(file, line); 

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        // 使用 Utils::split 分割 CSV 行
        std::vector<std::string> data = Utils::split(line, ',');
        
        if (data.size() >= 7) {
            try {
                int id = std::stoi(data[0]);
                std::string name = data[1];
                std::string belonging = data[2];
                std::string sending = data[3];
                int type = std::stoi(data[4]); 
                int grade = std::stoi(data[5]);
                std::string date = data[6];

                Goods goods(id, name, belonging, sending, type, grade, date);
                
                // 将货物放入对应的方案树节点中
                // 如果 type 在 1-4 之间，放入对应节点；否则放入默认(比如综合 3)
                int targetScheme = (type >= 1 && type <= 4) ? type : 3;
                logisticsTree[targetScheme].goodsList.push_back(goods);
                
                // 收集所有出现的地区
                regionSet.insert(belonging);
                regionSet.insert(sending); // 发往地也应该算作已知地区
            } catch (...) { 
                continue; 
            }
        }
    }
    
    // 将 set 转回 vector 供全局使用
    belongingRegionList.assign(regionSet.begin(), regionSet.end());
    std::cout << "数据读取完成，共加载地区数: " << belongingRegionList.size() << std::endl;
}