#include "Dijkstra.h"
#include <iostream>
#include <climits>
#include <algorithm>

void Dijkstra::minStep(const std::vector<std::vector<int>>& weight, 
                       int start, int end, 
                       const std::vector<std::string>& regions) {
    int n = weight.size();
    std::vector<int> shortPath(n);
    std::vector<std::string> path(n);
    std::vector<int> visited(n, 0);

    // 初始化
    for (int i = 0; i < n; i++) {
        path[i] = regions[start] + "-->" + regions[i];
        shortPath[i] = weight[start][i];
    }
    
    shortPath[start] = 0;
    visited[start] = 1;

    for (int count = 1; count < n; count++) {
        int k = -1;
        int dmin = INT_MAX;
        
        for (int i = 0; i < n; i++) {
            if (visited[i] == 0 && shortPath[i] < dmin) {
                dmin = shortPath[i];
                k = i;
            }
        }

        if (k == -1) break; // 无法到达其他节点

        visited[k] = 1;

        for (int i = 0; i < n; i++) {
            // 这里假设 weight[k][i] < INT_MAX
            if (visited[i] == 0 && weight[k][i] < INT_MAX) {
                if (dmin + weight[k][i] < shortPath[i]) {
                    shortPath[i] = dmin + weight[k][i];
                    path[i] = path[k] + "-->" + regions[i];
                }
            }
        }
    }

    if (end == -1) {
        for (int i = 0; i < n; i++) {
            if (shortPath[i] >= 10000) { // 10000代表不可达
                 std::cout << "从" << regions[start] << "到" << regions[i] << "不可达" << std::endl;
            } else {
                std::cout << "从" << regions[start] << "出发到" << regions[i] 
                          << "的最短路径为：" << path[i] 
                          << "，最短距离为：" << shortPath[i] << std::endl;
            }
        }
        std::cout << "=====================================" << std::endl;
    } else {
         std::cout << "从" << regions[start] << "出发到" << regions[end] 
                   << "的最短路径为：" << path[end] 
                   << "，最短距离为：" << shortPath[end] << std::endl;
    }
}