#include "Goods.h"
#include "Utils.h"

Goods::Goods(int id, std::string name, std::string belongingArea, 
             std::string sendingArea, int type, int clientGrade, std::string date)
    : id(id), name(name), belongingArea(belongingArea), 
      sendingArea(sendingArea), type(type), clientGrade(clientGrade), dateStr(date) {
    calculatePriority();
}

void Goods::calculatePriority() {
    float typeWeight = 0.2f;
    float clientGradeWeight = 0.3f;
    float dateWeight = 0.4f;
    
    double days = Utils::daysFromToday(dateStr);
    this->priority = type * typeWeight + clientGrade * clientGradeWeight + days * dateWeight;
}

std::ostream& operator<<(std::ostream& os, const Goods& g) {
    os << "Goods{ID=" << g.id 
       << ", name='" << g.name << "'"
       << ", belongingArea='" << g.belongingArea << "'"
       << ", sendingArea='" << g.sendingArea << "'"
       << ", type=" << g.type 
       << ", clientGrade=" << g.clientGrade 
       << ", date='" << g.dateStr << "'"
       << ", priority=" << Utils::formatDouble(g.priority) << "}";
    return os;
}