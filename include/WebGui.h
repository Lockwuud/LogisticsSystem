#ifndef WEB_GUI_H
#define WEB_GUI_H

#include <string>
#include <vector>
#include <map>

class WebGui {
public:
    void start();
    void init();

private:
    void handleClient(int clientSocket);
    
    // --- 实验一 API ---
    std::string handleApiRegions();
    std::string handleApiGoods(const std::map<std::string, std::string>& params);
    std::string handleApiShip(const std::map<std::string, std::string>& params);
    std::string handleApiPath(const std::map<std::string, std::string>& params);
    
    // --- 实验二 API ---
    std::string handleApiDbList(); 
    std::string handleApiDbCreate(const std::map<std::string, std::string>& params); 
    std::string handleApiDbInsert(const std::map<std::string, std::string>& params); 
    std::string handleApiDbQuery(const std::map<std::string, std::string>& params);

    // --- 实验四 API ---
    std::string handleApiReload(); // 重载数据
    std::string handleApiStatus(const std::map<std::string, std::string>& params); // 物流状态查询

    // 辅助
    std::map<std::string, std::string> parseQuery(const std::string& query);
    std::vector<std::vector<int>> getGraphForScheme(int schemeId);
    void sendResponse(int socket, const std::string& content, const std::string& contentType);

    void loadDataInternal();
};

#endif