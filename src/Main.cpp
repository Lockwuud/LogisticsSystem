#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <set>
#include <map>
#include <sstream>
#include "Goods.h"
#include "Utils.h"
#include "Dijkstra.h"

// --- 1. 定义树型结构 ---
// 结构：Root -> 方案节点(1,2,3,4) -> 货物列表(Leaf)
struct SchemeNode {
    int schemeId;           // 1:价格最小, 2:时间最短, 3:综合最优, 4:航空
    std::string schemeName;
    std::vector<Goods> goodsList; // 该方案下的货物
};

// 全局的树结构 (0号下标留空，方便对应ID 1-4)
SchemeNode logisticsTree[5]; 
std::vector<std::string> belongingRegionList;

// 前向声明
void readDataFromFile(const std::string& filename);
void regionSelector(int regionIndex);
void prioritySort(const std::string& region);
void projectSelector(const std::string& region, int projectId);
void printGoodsOfThisProject(const std::string& region, int projectId);
void minStepWrapper(int start, int end, int projectId);
void sendGoods(const std::string& region, int projectId);
std::vector<std::vector<int>> readMatrix(const std::string& filename, int size);

int main() {
    // UTF-8 输出
    #ifdef _WIN32
    system("chcp 65001");
    #endif

    // 初始化方案树的名称
    logisticsTree[1] = {1, "价格最小方案", {}};
    logisticsTree[2] = {2, "时间最短方案", {}};
    logisticsTree[3] = {3, "综合最优方案", {}};
    logisticsTree[4] = {4, "航空物流方案", {}};

    std::cout << "------------欢迎来到物流管理系统-----------" << std::endl;
    std::cout << "读取数据中..." << std::endl;

    readDataFromFile("../data/goodsInfo.csv"); 

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

void readDataFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }

    std::string line;
    std::set<std::string> regionSet;
    std::getline(file, line); // 跳过表头

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<std::string> data = Utils::split(line, ',');
        
        if (data.size() >= 7) {
            try {
                int id = std::stoi(data[0]);
                std::string name = data[1];
                std::string belonging = data[2];
                std::string sending = data[3];
                int type = std::stoi(data[4]); // 这里 type 对应方案 ID
                int grade = std::stoi(data[5]);
                std::string date = data[6];

                Goods goods(id, name, belonging, sending, type, grade, date);
                
                // --- 核心修改：直接归类到树型结构中 ---
                // 如果 type 在 1-4 之间，放入对应节点；否则放入默认(比如综合 3)
                int targetScheme = (type >= 1 && type <= 4) ? type : 3;
                logisticsTree[targetScheme].goodsList.push_back(goods);
                
                regionSet.insert(belonging);
            } catch (...) { continue; }
        }
    }
    belongingRegionList.assign(regionSet.begin(), regionSet.end());
}

// 辅助函数：读取 CSV 矩阵
std::vector<std::vector<int>> readMatrix(const std::string& filename, int size) {
    std::vector<std::vector<int>> matrix(size, std::vector<int>(size, 10000));
    std::ifstream file(filename);
    if (!file.is_open()) return matrix; // 返回全无穷大矩阵

    std::string line;
    std::vector<std::string> csvRegions;

    // 1. 读取表头获取 CSV 中的地区顺序
    if (std::getline(file, line)) {
         if (!line.empty() && line.back() == '\r') line.pop_back();
         std::vector<std::string> headers = Utils::split(line, ',');
         for (size_t i = 1; i < headers.size(); ++i) {
             std::string name = headers[i];
             // 简单去空
             name.erase(0, name.find_first_not_of(" \t\r\n"));
             name.erase(name.find_last_not_of(" \t\r\n") + 1);
             if(!name.empty()) csvRegions.push_back(name);
         }
    }

    // 2. 建立映射: CSV索引 -> 全局索引
    std::vector<int> mapCsvToGlobal(csvRegions.size(), -1);
    for(size_t i=0; i<csvRegions.size(); ++i) {
        auto it = std::find(belongingRegionList.begin(), belongingRegionList.end(), csvRegions[i]);
        if (it != belongingRegionList.end()) {
            mapCsvToGlobal[i] = std::distance(belongingRegionList.begin(), it);
        }
    }

    // 3. 读取并填充
    int rowCount = 0;
    while(std::getline(file, line) && rowCount < csvRegions.size()) {
         if (!line.empty() && line.back() == '\r') line.pop_back();
         std::vector<std::string> tokens = Utils::split(line, ',');
         if (tokens.size() > 1) {
             for (size_t col = 0; col < csvRegions.size(); ++col) {
                 if (col + 1 < tokens.size()) {
                     int u = mapCsvToGlobal[rowCount];
                     int v = mapCsvToGlobal[col];
                     if (u != -1 && v != -1) {
                         try {
                             std::string val = tokens[col+1];
                             if(!val.empty()) matrix[u][v] = std::stoi(val);
                         } catch(...) {}
                     }
                 }
             }
             rowCount++;
         }
    }
    return matrix;
}

void minStepWrapper(int start, int end, int projectId) {
    int size = belongingRegionList.size();
    std::vector<std::vector<int>> finalGraph;

    // --- 核心修改：根据方案 ID 加载不同矩阵 ---
    if (projectId == 1) {
        // 方案1：价格最小 -> 读取 regionPrice.csv
        finalGraph = readMatrix("../data/regionPrice.csv", size);
        std::cout << "[当前使用的是价格矩阵]" << std::endl;
    } 
    else if (projectId == 2) {
        // 方案2：时间最短 -> 读取 regionDistance.csv
        finalGraph = readMatrix("../data/regionDistance.csv", size);
        std::cout << "[当前使用的是时间矩阵]" << std::endl;
    } 
    else if (projectId == 4) {
        // 方案4：航空 -> 读取 regionAir.csv
        finalGraph = readMatrix("../data/regionAir.csv", size);
        std::cout << "[当前使用的是航空矩阵]" << std::endl;
    }
    else {
        // 方案3：综合最优 -> 价格 + 时间 (加权)
        // 读取两个矩阵
        auto timeMatrix = readMatrix("../data/regionDistance.csv", size);
        auto priceMatrix = readMatrix("../data/regionPrice.csv", size);
        finalGraph.resize(size, std::vector<int>(size));
        
        // 简单的综合公式：代价 = 时间 + 0.1 * 价格 (归一化权衡)
        std::cout << "[当前使用的是综合最优矩阵 (时间 + 0.1*价格)]" << std::endl;
        for(int i=0; i<size; ++i) {
            for(int j=0; j<size; ++j) {
                // 如果任意一个是不可达(10000)，则结果不可达
                if (timeMatrix[i][j] >= 10000 || priceMatrix[i][j] >= 10000) {
                    finalGraph[i][j] = 10000;
                } else {
                    finalGraph[i][j] = timeMatrix[i][j] + (int)(priceMatrix[i][j] * 0.1);
                }
            }
        }
    }

    // 调用 Dijkstra
    Dijkstra::minStep(finalGraph, start, end, belongingRegionList);
}

void regionSelector(int regionIndex) {
    std::string currentRegion = belongingRegionList[regionIndex];
    std::cout << "----------地区" << currentRegion << "----------" << std::endl;
    
    while (true) {
        std::cout << "1.按照优先级别排序物品(查看本地所有)\n"
                  << "2.价格最小物流方案 (Type 1)\n"
                  << "3.时间最短物流方案 (Type 2)\n"
                  << "4.综合物流最优方案 (Type 3)\n"
                  << "5.航空物流方案 (Type 4)\n\n"
                  << "0.返回上一页" << std::endl;
        
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
    // 这里的逻辑可以保留为查看所有（不分方案），或者遍历树的所有节点
    std::vector<Goods> allLocalGoods;
    
    // 遍历树的4个方案节点，收集所有属于当前 region 的货物
    for(int i=1; i<=4; ++i) {
        for(const auto& g : logisticsTree[i].goodsList) {
            if (g.belongingArea == region) {
                allLocalGoods.push_back(g);
            }
        }
    }

    std::sort(allLocalGoods.begin(), allLocalGoods.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority; 
    });

    for (const auto& g : allLocalGoods) {
        std::cout << g << std::endl;
    }
    std::cout << std::endl;
}

void projectSelector(const std::string& region, int projectId) {
    // projectId 对应树的下标 (1-4)
    std::cout << "当前方案：" << logisticsTree[projectId].schemeName << std::endl;

    while (true) {
        std::cout << "1.按优先级输出该方案的物品\n2.输出最短路径\n3.根据物品ID发货\n\n0.返回上一页" << std::endl;
        int val;
        std::cin >> val;
        switch (val) {
            case 0: return;
            case 1: printGoodsOfThisProject(region, projectId); break;
            case 2: 
                {
                    auto it = std::find(belongingRegionList.begin(), belongingRegionList.end(), region);
                    int idx = std::distance(belongingRegionList.begin(), it);
                    minStepWrapper(idx, -1, projectId);
                }
                break;
            case 3: sendGoods(region, projectId); break;
            default: std::cout << "输入错误" << std::endl;
        }
    }
}

void printGoodsOfThisProject(const std::string& region, int projectId) {
    // --- 核心修改：直接从树节点中获取数据 ---
    // 获取该方案节点下的所有货物副本
    std::vector<Goods> filtered = logisticsTree[projectId].goodsList;
    
    // 进一步过滤：必须是当前地区的
    std::vector<Goods> localFiltered;
    for(const auto& g : filtered) {
        if (g.belongingArea == region) {
            localFiltered.push_back(g);
        }
    }
    
    std::sort(localFiltered.begin(), localFiltered.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority;
    });

    if (localFiltered.empty()) {
        std::cout << "该地区下暂无属于此方案的货物。" << std::endl;
    } else {
        for (const auto& g : localFiltered) {
            std::cout << g << std::endl;
        }
    }
    std::cout << std::endl;
}

void sendGoods(const std::string& region, int projectId) {
    while(true) {
        std::cout << "请输入物品ID发货，输入0返回上一层：" << std::endl;
        int id;
        std::cin >> id;
        if (id == 0) return;
        
        bool found = false;
        // --- 核心修改：只在当前方案树节点中查找 ---
        const auto& currentSchemeGoods = logisticsTree[projectId].goodsList;
        
        for (const auto& g : currentSchemeGoods) {
            if (g.belongingArea == region && g.id == id) {
                std::cout << "正在处理货物: " << g << std::endl;
                
                int startIdx = -1, endIdx = -1;
                auto itStart = std::find(belongingRegionList.begin(), belongingRegionList.end(), region);
                auto itEnd = std::find(belongingRegionList.begin(), belongingRegionList.end(), g.sendingArea);
                
                if (itStart != belongingRegionList.end()) startIdx = std::distance(belongingRegionList.begin(), itStart);
                if (itEnd != belongingRegionList.end()) endIdx = std::distance(belongingRegionList.begin(), itEnd);

                if (startIdx != -1 && endIdx != -1) {
                    minStepWrapper(startIdx, endIdx, projectId);
                }
                found = true;
                break; // ID唯一，找到即可退出
            }
        }
        if(!found) std::cout << "未找到该ID的物品或该物品不属于当前方案。" << std::endl;
    }
}