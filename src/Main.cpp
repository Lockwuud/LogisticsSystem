#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <set>
#include <sstream>
#include <windows.h>
#include "Goods.h"
#include "Utils.h"
#include "Dijkstra.h"

// 全局变量
std::vector<Goods> goodsList;
std::vector<std::string> belongingRegionList;

// 前向声明函数
void readDataFromFile(const std::string& filename);
void regionSelector(int regionIndex);
void prioritySort(const std::string& region);
void projectSelector(const std::string& region, int projectId);
void printGoodsOfThisProject(const std::string& region, int goodsType);
void minStepWrapper(int start, int end, int projectId);
void sendGoods(const std::string& region, int projectId);

int main() {
    SetConsoleOutputCP(65001);

    std::cout << "------------欢迎来到物流管理系统(C++版)-----------" << std::endl;
    std::cout << "读取数据中..." << std::endl;

    // 注意：请确保 data 文件夹与可执行文件在相对路径正确的位置
    // 或者使用绝对路径
    readDataFromFile("../data/goodsInfo_generated.csv"); 
    while (true) {
        std::cout << "----------请选择地区----------" << std::endl;
        for (size_t i = 0; i < belongingRegionList.size(); i++) {
            std::cout << i + 1 << "." << belongingRegionList[i] << std::endl;
        }
        std::cout << "\n0.退出系统" << std::endl;

        int inputValue;
        if (std::cin >> inputValue) {
            if (inputValue == 0) {
                std::cout << "--------------系统登出-------------" << std::endl;
                break;
            } else if (inputValue > 0 && inputValue <= (int)belongingRegionList.size()) {
                regionSelector(inputValue - 1);
            } else {
                std::cout << "输入错误！请重新输入！" << std::endl;
            }
        } else {
             std::cin.clear();
             std::cin.ignore(10000, '\n');
             std::cout << "输入错误！" << std::endl;
        }
    }
    return 0;
}

// 读取 CSV 数据
void readDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << "，请确认路径是否正确且已转换为CSV格式。" << std::endl;
        return;
    }

    std::string line;
    std::set<std::string> regionSet;
    
    // 跳过标题行 
    std::getline(file, line); 

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> data = Utils::split(line, ',');
        
        // CSV 格式: ID, Name, Belonging, Sending, Type, Grade, Date
        if (data.size() >= 7) {
            try {
                int id = std::stoi(data[0]);
                std::string name = data[1];
                std::string belonging = data[2];
                std::string sending = data[3];
                int type = std::stoi(data[4]);
                int grade = std::stoi(data[5]);
                std::string date = data[6];

                goodsList.emplace_back(id, name, belonging, sending, type, grade, date);
                regionSet.insert(belonging);
            } catch (...) {
                continue; // 忽略解析错误的行
            }
        }
    }
    
    belongingRegionList.assign(regionSet.begin(), regionSet.end());
}

void regionSelector(int regionIndex) {
    std::string currentRegion = belongingRegionList[regionIndex];
    std::cout << "----------地区" << currentRegion << "----------" << std::endl;
    
    while (true) {
        std::cout << "1.按照优先级别排序物品\n2.价格最小物流方案\n3.时间最短物流方案\n"
                  << "4.综合物流最优方案\n5.航空物流方案\n\n0.返回上一页" << std::endl;
        
        int val;
        std::cin >> val;
        switch (val) {
            case 0: return;
            case 1: prioritySort(currentRegion); break;
            case 2: projectSelector(currentRegion, 1); break;
            case 3: projectSelector(currentRegion, 2); break;
            case 4: projectSelector(currentRegion, 3); break;
            case 5: projectSelector(currentRegion, 4); break;
            default: std::cout << "输入错误" << std::endl;
        }
    }
}

void prioritySort(const std::string& region) {
    // 过滤出该地区的商品
    std::vector<Goods> filtered;
    for (const auto& g : goodsList) {
        if (g.belongingArea == region) {
            filtered.push_back(g);
        }
    }
    
    // 排序 lambda 表达式
    std::sort(filtered.begin(), filtered.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority; // 降序
    });

    for (const auto& g : filtered) {
        std::cout << g << std::endl;
    }
    std::cout << std::endl;
}

void projectSelector(const std::string& region, int projectId) {
    while (true) {
        std::cout << "1.按优先级输出该方案的物品\n2.输出最短路径\n3.根据物品ID发货\n\n0.返回上一页" << std::endl;
        int val;
        std::cin >> val;
        switch (val) {
            case 0: return;
            case 1: printGoodsOfThisProject(region, projectId); break;
            case 2: minStepWrapper(distance(belongingRegionList.begin(), 
                                   find(belongingRegionList.begin(), belongingRegionList.end(), region)), -1, projectId); break;
            case 3: sendGoods(region, projectId); break;
            default: std::cout << "输入错误" << std::endl;
        }
    }
}

void printGoodsOfThisProject(const std::string& region, int goodsType) {
    std::vector<Goods> filtered;
    for (const auto& g : goodsList) {
        if (g.belongingArea == region && g.type == goodsType) {
            filtered.push_back(g);
        }
    }
    
    std::sort(filtered.begin(), filtered.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority;
    });

    for (const auto& g : filtered) {
        std::cout << g << std::endl;
    }
    std::cout << std::endl;
}

void minStepWrapper(int start, int end, int projectId) {
    // 1. 打开文件
    std::string filename = "../data/regionDistance.csv";
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件 " << filename << std::endl;
        return;
    }

    std::vector<std::vector<int>> rawGraph; // 暂存 CSV 里的原始矩阵
    std::vector<std::string> csvRegions;    // 暂存 CSV 里的地区名字
    std::string line;

    // 2. 读取第一行（列头）：例如 ",A,B,C,D,E,F,G,H"
    if (std::getline(file, line)) {
        // 处理 Windows 可能遗留的 \r 字符
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::vector<std::string> headers = Utils::split(line, ',');
        
        // headers[0] 是空的，从 headers[1] 开始是地区名
        for (size_t i = 1; i < headers.size(); ++i) {
            // 简单清洗一下名字（去掉可能存在的空格）
            std::string name = headers[i];
            name.erase(0, name.find_first_not_of(" \t\r\n"));
            name.erase(name.find_last_not_of(" \t\r\n") + 1);
            if (!name.empty()) {
                csvRegions.push_back(name);
            }
        }
    }

    int n = csvRegions.size(); // CSV 中定义的地区数量
    rawGraph.resize(n, std::vector<int>(n, 10000)); // 初始化为无穷大

    // 3. 读取数据行
    int rowCount = 0;
    while (std::getline(file, line) && rowCount < n) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        // 跳过空行或只有逗号的行
        bool validLine = false;
        for (char c : line) if (c != ',' && c != ' ') validLine = true;
        if (!validLine) continue;

        std::vector<std::string> tokens = Utils::split(line, ',');
        
        // 这一行的第一列是行名（如 "A"），后面是数据
        // 数据从 tokens[1] 开始
        if (tokens.size() > 1) {
            for (int col = 0; col < n; ++col) {
                if (col + 1 < tokens.size()) {
                    try {
                        std::string valStr = tokens[col + 1];
                        if (valStr.empty()) {
                            rawGraph[rowCount][col] = 10000;
                        } else {
                            rawGraph[rowCount][col] = std::stoi(valStr);
                        }
                    } catch (...) {
                        rawGraph[rowCount][col] = 10000; // 解析失败当作不可达
                    }
                }
            }
            rowCount++;
        }
    }

    // 4. 【关键】对齐矩阵
    // 程序里的 belongingRegionList 顺序可能和 CSV 不一样 (比如 CSV 是 A,B... 而列表里是 B,A...)
    // 我们需要构建一个基于 belongingRegionList 的最终矩阵
    int globalSize = belongingRegionList.size();
    std::vector<std::vector<int>> finalGraph(globalSize, std::vector<int>(globalSize, 10000));

    // 建立映射：CSV 中的第 i 个地区 -> 全局列表中的第 index 个
    std::vector<int> mapCsvToGlobal(n, -1);
    for(int i = 0; i < n; ++i) {
        // 在全局列表中查找 CSV 里的地区名
        auto it = std::find(belongingRegionList.begin(), belongingRegionList.end(), csvRegions[i]);
        if (it != belongingRegionList.end()) {
            mapCsvToGlobal[i] = std::distance(belongingRegionList.begin(), it);
        }
    }

    // 填充最终矩阵
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            int u = mapCsvToGlobal[i]; // 源
            int v = mapCsvToGlobal[j]; // 目的
            
            // 只有当源和目的都在我们的全局列表中存在时，才复制距离
            if (u != -1 && v != -1) {
                finalGraph[u][v] = rawGraph[i][j];
            }
        }
    }

    // 5. 调用 Dijkstra
    // 这里的 start 和 end 是基于 belongingRegionList 的索引，可以直接传入 finalGraph
    Dijkstra::minStep(finalGraph, start, end, belongingRegionList);
    std::cout << std::endl;
}

void sendGoods(const std::string& region, int projectId) {
    while(true) {
        std::cout << "请输入物品ID发货，输入0返回上一层：" << std::endl;
        int id;
        std::cin >> id;
        if (id == 0) return;
        
        bool found = false;
        for (const auto& g : goodsList) {
            if (g.belongingArea == region && g.type == projectId && g.id == id) {
                std::cout << g << std::endl;
                
                int startIdx = -1, endIdx = -1;
                auto itStart = std::find(belongingRegionList.begin(), belongingRegionList.end(), region);
                auto itEnd = std::find(belongingRegionList.begin(), belongingRegionList.end(), g.sendingArea);
                
                if (itStart != belongingRegionList.end()) startIdx = std::distance(belongingRegionList.begin(), itStart);
                if (itEnd != belongingRegionList.end()) endIdx = std::distance(belongingRegionList.begin(), itEnd);

                if (startIdx != -1 && endIdx != -1) {
                    minStepWrapper(startIdx, endIdx, projectId);
                }
                found = true;
            }
        }
        if(!found) std::cout << "未找到该ID的物品或不符合当前方案" << std::endl;
    }
}