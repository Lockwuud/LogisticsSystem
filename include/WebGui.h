#ifndef WEB_GUI_H
#define WEB_GUI_H

#include <string>
#include <vector>
#include <map>

class WebGui {
public:
    void start();

private:
    void handleClient(int clientSocket);
    
    // API 处理函数
    std::string handleApiRegions();
    std::string handleApiGoods(const std::map<std::string, std::string>& params);
    std::string handleApiShip(const std::map<std::string, std::string>& params);
    std::string handleApiPath(const std::map<std::string, std::string>& params);
    
    // 辅助函数
    std::map<std::string, std::string> parseQuery(const std::string& query);
    std::vector<std::vector<int>> getGraphForScheme(int schemeId);
    
    // 响应构建
    void sendResponse(int socket, const std::string& content, const std::string& contentType);
};

#endif