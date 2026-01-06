#include "WebGui.h"
#include "GlobalData.h"
#include "Utils.h"
#include "Dijkstra.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

// 如果你不想创建 index.html 文件，可以将上面的 HTML 代码粘贴在这个字符串里
const std::string FALLBACK_HTML = R"(
<!DOCTYPE html><html><body><h1>请在同级目录下创建 index.html 文件以加载完整界面。</h1></body></html>
)";

void WebGui::start() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Web GUI 服务已启动: http://localhost:8080" << std::endl;
    std::cout << "现在你可以关闭控制台窗口，在浏览器中进行所有操作。" << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
            closesocket(clientSocket);
        }
    }
    WSACleanup();
}

void WebGui::handleClient(int clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    if (bytesReceived <= 0) return;

    std::string request(buffer, bytesReceived);
    std::istringstream iss(request);
    std::string method, url;
    iss >> method >> url;

    // 分离 URL 和 参数
    std::string path = url;
    std::string query = "";
    size_t qPos = url.find('?');
    if (qPos != std::string::npos) {
        path = url.substr(0, qPos);
        query = url.substr(qPos + 1);
    }
    auto params = parseQuery(query);

    std::string responseContent;
    std::string contentType = "text/html; charset=utf-8";

    // 简单的路由系统
    if (path == "/") {
        // 尝试读取 index.html
        std::ifstream f("index.html");
        if (f.is_open()) {
            std::stringstream buffer;
            buffer << f.rdbuf();
            responseContent = buffer.str();
        } else {
            // 如果没找到文件，尝试去 src 找或者使用备用
            std::ifstream fSrc("../src/index.html"); // 假设 build 目录结构
            if(fSrc.is_open()){
                std::stringstream buffer;
                buffer << fSrc.rdbuf();
                responseContent = buffer.str();
            } else {
                responseContent = FALLBACK_HTML;
            }
        }
    } 
    else if (path == "/api/regions") {
        responseContent = handleApiRegions();
        contentType = "application/json";
    }
    else if (path == "/api/goods") {
        responseContent = handleApiGoods(params);
        contentType = "application/json";
    }
    else if (path == "/api/ship") {
        responseContent = handleApiShip(params);
        contentType = "application/json";
    }
    else if (path == "/api/path") {
        responseContent = handleApiPath(params);
        contentType = "application/json";
    }
    
    sendResponse(clientSocket, responseContent, contentType);
}

// ------ API 处理器 ------

std::string WebGui::handleApiRegions() {
    // 手动构建 JSON 数组
    std::string json = "[";
    for (size_t i = 0; i < belongingRegionList.size(); ++i) {
        json += "\"" + belongingRegionList[i] + "\"";
        if (i < belongingRegionList.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

std::string WebGui::handleApiGoods(const std::map<std::string, std::string>& params) {
    std::string region = "";
    int scheme = 1;
    if (params.count("region")) region = params.at("region");
    if (params.count("scheme")) scheme = std::stoi(params.at("scheme"));

    if (scheme < 1 || scheme > 4) scheme = 3;

    auto list = logisticsTree[scheme].goodsList;
    std::vector<Goods> filtered;
    
    for (const auto& g : list) {
        if (g.belongingArea == region) filtered.push_back(g);
    }
    
    // 排序
    std::sort(filtered.begin(), filtered.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority;
    });

    // 构建 JSON
    std::string json = "[";
    for (size_t i = 0; i < filtered.size(); ++i) {
        const auto& g = filtered[i];
        json += "{";
        json += "\"id\":" + std::to_string(g.id) + ",";
        json += "\"name\":\"" + g.name + "\",";
        json += "\"sendingArea\":\"" + g.sendingArea + "\",";
        json += "\"priority\":\"" + Utils::formatDouble(g.priority) + "\"";
        json += "}";
        if (i < filtered.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

std::string WebGui::handleApiShip(const std::map<std::string, std::string>& params) {
    int id = 0; 
    int scheme = 1;
    if (params.count("id")) id = std::stoi(params.at("id"));
    if (params.count("scheme")) scheme = std::stoi(params.at("scheme"));

    bool found = false;
    if (scheme >= 1 && scheme <= 4) {
        auto& list = logisticsTree[scheme].goodsList;
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->id == id) {
                list.erase(it);
                found = true;
                break;
            }
        }
    }
    return found ? "{\"success\":true}" : "{\"success\":false}";
}

std::string WebGui::handleApiPath(const std::map<std::string, std::string>& params) {
    std::string startRegion = "", endRegion = "";
    int scheme = 1;
    if (params.count("start")) startRegion = params.at("start");
    if (params.count("end")) endRegion = params.at("end");
    if (params.count("scheme")) scheme = std::stoi(params.at("scheme"));

    int u = -1, v = -1;
    auto it1 = std::find(belongingRegionList.begin(), belongingRegionList.end(), startRegion);
    if(it1 != belongingRegionList.end()) u = std::distance(belongingRegionList.begin(), it1);
    
    auto it2 = std::find(belongingRegionList.begin(), belongingRegionList.end(), endRegion);
    if(it2 != belongingRegionList.end()) v = std::distance(belongingRegionList.begin(), it2);

    std::string resultText = "无法识别地区";
    if (u != -1 && v != -1) {
        auto matrix = getGraphForScheme(scheme);
        resultText = Dijkstra::minStep(matrix, u, v, belongingRegionList);
    }
    
    // 对结果中的换行符进行转义，以符合 JSON 格式
    std::string escaped;
    for (char c : resultText) {
        if (c == '\n') escaped += "\\n";
        else escaped += c;
    }

    return "{\"result\":\"" + escaped + "\"}";
}

// ------ 辅助函数 ------

std::map<std::string, std::string> WebGui::parseQuery(const std::string& query) {
    std::map<std::string, std::string> params;
    std::vector<std::string> pairs = Utils::split(query, '&');
    for (const auto& pair : pairs) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
            // 简单的 URL 解码
            std::string decoded;
            for (size_t i = 0; i < val.length(); i++) {
                if (val[i] == '%' && i + 2 < val.length()) {
                    int hex;
                    std::sscanf(val.substr(i + 1, 2).c_str(), "%x", &hex);
                    decoded += static_cast<char>(hex);
                    i += 2;
                } else if (val[i] == '+') {
                    decoded += ' ';
                } else {
                    decoded += val[i];
                }
            }
            params[key] = decoded;
        }
    }
    return params;
}

std::vector<std::vector<int>> WebGui::getGraphForScheme(int schemeId) {
    int size = belongingRegionList.size();
    if (schemeId == 1) return Utils::readMatrix("../data/regionPrice.csv", size, belongingRegionList);
    if (schemeId == 2) return Utils::readMatrix("../data/regionDistance.csv", size, belongingRegionList);
    if (schemeId == 4) return Utils::readMatrix("../data/regionAir.csv", size, belongingRegionList);
    
    // 方案3 综合
    auto timeM = Utils::readMatrix("../data/regionDistance.csv", size, belongingRegionList);
    auto priceM = Utils::readMatrix("../data/regionPrice.csv", size, belongingRegionList);
    std::vector<std::vector<int>> combined(size, std::vector<int>(size));
    
    for(int i=0; i<size; ++i) {
        for(int j=0; j<size; ++j) {
            if(timeM[i][j] >= 10000 || priceM[i][j] >= 10000) combined[i][j] = 10000;
            else combined[i][j] = timeM[i][j] + (int)(priceM[i][j] * 0.1);
        }
    }
    return combined;
}

void WebGui::sendResponse(int socket, const std::string& content, const std::string& contentType) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    send(socket, response.c_str(), response.size(), 0);
}