#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <vector>
#include <string>

class Dijkstra {
public:
    // 计算最短路径
    // weight: 邻接矩阵, start: 起点索引, end: 终点索引(-1代表打印所有), regions: 地区名列表
    static void minStep(const std::vector<std::vector<int>>& weight, 
                        int start, int end, 
                        const std::vector<std::string>& regions);
};

#endif // DIJKSTRA_H