#include "WebGui.h"
#include "GlobalData.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h> // for socklen_t

// 链接 ws2_32.lib 是在 CMakeLists.txt 中完成的

void WebGui::start() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "无法创建 Socket." << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080); // 端口设置为 8080

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "端口绑定失败，请确保8080端口未被占用。" << std::endl;
        closesocket(serverSocket);
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        return;
    }

    std::cout << "\n==============================================" << std::endl;
    std::cout << "  Web GUI 已启动! 请在浏览器访问: http://localhost:8080" << std::endl;
    std::cout << "  按 Ctrl+C 可以在控制台强制结束" << std::endl;
    std::cout << "==============================================\n" << std::endl;

    while (true) {
        SOCKET clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
            closesocket(clientSocket);
        }
    }
    
    closesocket(serverSocket);
    WSACleanup();
}

void WebGui::handleClient(int clientSocket) {
    char buffer[4096];
    int bytesReceived = recv(clientSocket, buffer, 4096, 0);
    if (bytesReceived <= 0) return;

    std::string request(buffer, bytesReceived);
    
    // 简单的路由判断
    // 如果请求包含 "GET /ship?", 说明是点击了发货按钮
    if (request.find("GET /ship?") != std::string::npos) {
        handleShipRequest(request);
        // 发货后重定向回主页
        std::string response = "HTTP/1.1 302 Found\r\nLocation: /\r\n\r\n";
        send(clientSocket, response.c_str(), response.size(), 0);
    } 
    else {
        // 否则显示主页
        std::string html = generateDashboard();
        std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " 
                             + std::to_string(html.size()) + "\r\n\r\n" + html;
        send(clientSocket, response.c_str(), response.size(), 0);
    }
}

void WebGui::handleShipRequest(const std::string& request) {
    // 解析 URL 参数: /ship?type=1&id=101
    int type = getParam(request, "type");
    int id = getParam(request, "id");
    
    if (type >= 1 && type <= 4) {
        auto& list = logisticsTree[type].goodsList;
        // 使用 erase-remove idiom 删除指定ID的货物
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (it->id == id) {
                std::cout << "[Web操作] 已发货(删除) -> ID: " << id << " (方案: " << type << ")" << std::endl;
                list.erase(it);
                break;
            }
        }
    }
}

int WebGui::getParam(const std::string& request, const std::string& key) {
    std::string search = key + "=";
    size_t pos = request.find(search);
    if (pos == std::string::npos) return -1;
    
    size_t start = pos + search.length();
    size_t end = request.find_first_of("& ", start);
    std::string val = request.substr(start, end - start);
    return std::stoi(val);
}

std::string WebGui::generateDashboard() {
    std::stringstream ss;
    ss << "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>物流管理系统</title>";
    ss << "<style>";
    ss << "body { font-family: '微软雅黑', sans-serif; background-color: #f4f6f9; padding: 20px; }";
    ss << "h1 { text-align: center; color: #333; }";
    ss << ".card { background: white; margin-bottom: 20px; padding: 15px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
    ss << "h2 { border-bottom: 2px solid #007bff; padding-bottom: 10px; color: #007bff; }";
    ss << "table { width: 100%; border-collapse: collapse; margin-top: 10px; }";
    ss << "th, td { border: 1px solid #ddd; padding: 10px; text-align: center; }";
    ss << "th { background-color: #f8f9fa; font-weight: bold; }";
    ss << "tr:nth-child(even) { background-color: #f9f9f9; }";
    ss << ".btn { display: inline-block; padding: 5px 10px; color: white; background-color: #28a745; text-decoration: none; border-radius: 4px; }";
    ss << ".btn:hover { background-color: #218838; }";
    ss << "</style></head><body>";
    
    ss << "<h1>📦 智慧物流管理系统控制台</h1>";

    // 遍历4种方案，生成4个表格
    for (int i = 1; i <= 4; ++i) {
        ss << "<div class='card'>";
        ss << "<h2>" << logisticsTree[i].schemeName << " (方案Type: " << i << ")</h2>";
        
        if (logisticsTree[i].goodsList.empty()) {
            ss << "<p>暂无货物</p>";
        } else {
            ss << "<table>";
            ss << "<thead><tr><th>ID</th><th>名称</th><th>所属地</th><th>发往地</th><th>客户等级</th><th>接收日期</th><th>优先级</th><th>操作</th></tr></thead>";
            ss << "<tbody>";
            
            // 这里为了展示方便，我们可以临时排个序，或者直接显示
            // 如果想保持 C++ 端的排序，可以在这里先 sort 一下副本，但为了性能直接显示即可
            for (const auto& g : logisticsTree[i].goodsList) {
                ss << "<tr>";
                ss << "<td>" << g.id << "</td>";
                ss << "<td>" << g.name << "</td>";
                ss << "<td>" << g.belongingArea << "</td>";
                ss << "<td>" << g.sendingArea << "</td>";
                ss << "<td>" << g.clientGrade << "</td>";
                ss << "<td>" << g.dateStr << "</td>";
                ss << "<td>" << Utils::formatDouble(g.priority) << "</td>";
                // 发货按钮链接到 /ship?type=X&id=Y
                ss << "<td><a class='btn' href='/ship?type=" << i << "&id=" << g.id << "'>🚀 发货</a></td>";
                ss << "</tr>";
            }
            ss << "</tbody></table>";
        }
        ss << "</div>";
    }

    ss << "</body></html>";
    return ss.str();
}