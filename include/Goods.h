#ifndef GOODS_H
#define GOODS_H

#include <string>
#include <iostream>

class Goods {
public:
    int id;
    std::string name;
    std::string belongingArea;
    std::string sendingArea;
    int type;
    int clientGrade;
    std::string dateStr;
    double priority;

    Goods(int id, std::string name, std::string belongingArea, 
          std::string sendingArea, int type, int clientGrade, std::string date);

    // 计算优先级的逻辑
    void calculatePriority();
    
    // 打印信息
    friend std::ostream& operator<<(std::ostream& os, const Goods& g);
};

#endif // GOODS_H