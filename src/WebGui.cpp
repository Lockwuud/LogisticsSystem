#include "WebGui.h"
#include "GlobalData.h"
#include "Utils.h"
#include "Dijkstra.h"
#include "DBManager.h"
#include "SQLParser.h"
#include "Goods.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <set>

const std::string FALLBACK_HTML = R"(
<!DOCTYPE html><html><body><h1>请在同级目录下创建 index.html 文件以加载完整界面。</h1></body></html>
)";

void WebGui::init() {
    std::cout << "初始化系统数据..." << std::endl;
    loadDataInternal();
}

// 修复矩阵 CSV 的表头
void fixMatrixHeader(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return;
    std::string line;
    std::getline(in, line);
    std::vector<std::string> lines;
    lines.push_back(line);
    while (std::getline(in, line)) lines.push_back(line);
    in.close();

    // 如果第一行以逗号开头，说明缺少首列名
    if (!lines.empty() && !lines[0].empty() && lines[0][0] == ',') {
        lines[0] = "StartNode" + lines[0]; // 补全列名
        std::ofstream out(filename);
        for (const auto& l : lines) out << l << "\n";
        out.close();
        std::cout << "已自动修复矩阵表头: " << filename << std::endl;
    }
}

// 将数据读取逻辑封装，以便 SQL 修改后重载
void WebGui::loadDataInternal() {
    // 1. 修复三个矩阵文件的表头，以便 SQL 可以操作它们
    fixMatrixHeader("../data/regionPrice.csv");
    fixMatrixHeader("../data/regionDistance.csv");
    fixMatrixHeader("../data/regionAir.csv");

    // 读取 goodsInfo.csv
    for(int i=0; i<5; ++i) logisticsTree[i].goodsList.clear();
    belongingRegionList.clear();
    std::ifstream file("../data/goodsInfo.csv");

    if (!file.is_open()) return;
    std::string line;
    std::set<std::string> regionSet;
    std::getline(file, line); 
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();
        std::vector<std::string> data = Utils::split(line, ',');
        if (data.size() >= 7) {
            try {
                int id = std::stoi(data[0]);
                std::string name = data[1];
                std::string belonging = data[2];
                std::string sending = data[3];
                int type = std::stoi(data[4]);
                int grade = std::stoi(data[5]);
                std::string date = data[6];
                Goods goods(id, name, belonging, sending, type, grade, date);
                int targetScheme = (type >= 1 && type <= 4) ? type : 3;
                logisticsTree[targetScheme].goodsList.push_back(goods);
                regionSet.insert(belonging);
                regionSet.insert(sending);
            } catch (...) { continue; }
        }
    }
    belongingRegionList.assign(regionSet.begin(), regionSet.end());
    std::cout << "数据已重载，当前地区数: " << belongingRegionList.size() << std::endl;
}

void WebGui::start() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    // 允许端口重用，避免重启时提示端口被占用
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "绑定端口 8080 失败 (Error: " << WSAGetLastError() << ")" << std::endl;
        return;
    }
    
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

    // 路由分发
    if (path == "/") {
        std::ifstream f("index.html");
        if (!f.is_open()) f.open("../src/index.html");
        if (f.is_open()) {
            std::stringstream buffer; buffer << f.rdbuf();
            responseContent = buffer.str();
        } else responseContent = FALLBACK_HTML;
    } 
    // 实验一路由
    else if (path == "/api/regions") { responseContent = handleApiRegions(); contentType = "application/json"; }
    else if (path == "/api/goods") { responseContent = handleApiGoods(params); contentType = "application/json"; }
    else if (path == "/api/ship") { responseContent = handleApiShip(params); contentType = "application/json"; }
    else if (path == "/api/path") { responseContent = handleApiPath(params); contentType = "application/json"; }
    // 实验二路由
    else if (path == "/api/db/list") { responseContent = handleApiDbList(); contentType = "application/json"; }
    else if (path == "/api/db/create") { responseContent = handleApiDbCreate(params); contentType = "application/json"; }
    else if (path == "/api/db/insert") { responseContent = handleApiDbInsert(params); contentType = "application/json"; }
    else if (path == "/api/db/query") { responseContent = handleApiDbQuery(params); contentType = "application/json"; }
    // 实验三路由
    else if (path == "/api/sql/execute") {
        std::string sql = params.count("q") ? params.at("q") : "";
        responseContent = SQLParser::execute(sql);
        contentType = "application/json";
    }
    // 实验四
    else if (path == "/api/reload") { responseContent = handleApiReload(); contentType = "application/json"; }
    else if (path == "/api/status") { responseContent = handleApiStatus(params); contentType = "application/json"; }
    
    sendResponse(clientSocket, responseContent, contentType);
}

// ------ API 处理器 ------

// 获取所有物流地区的列表
std::string WebGui::handleApiRegions() {
    std::string json = "[";
    for (size_t i = 0; i < belongingRegionList.size(); ++i) {
        json += "\"" + belongingRegionList[i] + "\"";
        if (i < belongingRegionList.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

// 获取符合条件的货物列表
std::string WebGui::handleApiGoods(const std::map<std::string, std::string>& params) {
    std::string region = "";
    int scheme = 1;
    if (params.count("region")) region = params.at("region");
    if (params.count("scheme")) scheme = std::stoi(params.at("scheme"));

    if (scheme < 1 || scheme > 4) scheme = 3;

    auto list = logisticsTree[scheme].goodsList;
    std::vector<Goods> filtered;
    
    for (const auto& g : list) {
        // [新增功能] 如果 region 是 "ALL" 或空，则显示所有地区的货物
        if (region == "ALL" || region.empty() || g.belongingArea == region) {
            filtered.push_back(g);
        }
    }
    
    std::sort(filtered.begin(), filtered.end(), [](const Goods& a, const Goods& b) {
        return a.priority > b.priority;
    });

    std::string json = "[";
    for (size_t i = 0; i < filtered.size(); ++i) {
        const auto& g = filtered[i];
        json += "{";
        json += "\"id\":" + std::to_string(g.id) + ",";
        json += "\"name\":\"" + g.name + "\",";
        json += "\"belongingArea\":\"" + g.belongingArea + "\","; // [新增字段] 返回所属地
        json += "\"sendingArea\":\"" + g.sendingArea + "\",";
        json += "\"priority\":\"" + Utils::formatDouble(g.priority) + "\"";
        json += "}";
        if (i < filtered.size() - 1) json += ",";
    }
    json += "]";
    return json;
}

// 发货-从系统中删除货物
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
                // 修复：同步删除底层 CSV 文件中的记录
                // "goodsInfo" 是表名（对应 data/goodsInfo.csv）
                // "id" 是列名
                // std::to_string(id) 是要删除的值
                bool diskDeleteSuccess = DBManager::deleteRecords("goodsInfo", "物品ID", std::to_string(id));
                
                if (diskDeleteSuccess) {
                    // 只有磁盘删除成功了，才从内存删除，保证一致性
                    list.erase(it);
                    found = true;
                    std::cout << "[Info] 货物 ID " << id << " 已发货并从磁盘移除。" << std::endl;
                } else {
                    std::cerr << "[Error] 货物 ID " << id << " 发货失败：无法写入磁盘（可能被占用）。" << std::endl;
                }
                break;
            }
        }
    }
    return found ? "{\"success\":true}" : "{\"success\":false}";
}

// 计算两地之间的最短路径
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
    
    std::string escaped;
    for (char c : resultText) {
        if (c == '\n') escaped += "\\n";
        else escaped += c;
    }

    return "{\"result\":\"" + escaped + "\"}";
}

// --- 实验二新增实现 ---

// 列出 data 目录下所有的表
std::string WebGui::handleApiDbList() {
    auto tables = DBManager::getAllTables();
    std::string json = "[";
    for(size_t i=0; i<tables.size(); ++i) {
        json += "\"" + tables[i] + "\"";
        if(i < tables.size()-1) json += ",";
    }
    json += "]";
    return json;
}

// 创建一个新的数据表
std::string WebGui::handleApiDbCreate(const std::map<std::string, std::string>& params) {
    if(params.count("name") && params.count("cols")) {
        std::string name = params.at("name");
        std::string colsStr = params.at("cols");
        auto headers = Utils::split(colsStr, ',');
        bool success = DBManager::createTable(name, headers);
        return success ? "{\"success\":true}" : "{\"success\":false}";
    }
    return "{\"success\":false}";
}

// 向表中插入一条记录
std::string WebGui::handleApiDbInsert(const std::map<std::string, std::string>& params) {
    if(params.count("table") && params.count("data")) {
        std::string table = params.at("table");
        std::string data = params.at("data");
        bool success = DBManager::insertRecord(table, data);
        return success ? "{\"success\":true}" : "{\"success\":false}";
    }
    return "{\"success\":false}";
}

// 查询表中的数据
std::string WebGui::handleApiDbQuery(const std::map<std::string, std::string>& params) {
    std::string table = params.count("table") ? params.at("table") : "";
    std::string key = params.count("key") ? params.at("key") : "";
    
    // 解析列索引，默认为 -1 (表示全文搜索)
    int colIdx = -1;
    if (params.count("colIdx")) {
        try {
            colIdx = std::stoi(params.at("colIdx"));
        } catch(...) {
            colIdx = -1;
        }
    }
    
    return DBManager::query(table, key, colIdx);
}

// 系统热重载
std::string WebGui::handleApiReload() {
    loadDataInternal();
    return "{\"success\":true, \"msg\":\"系统数据已从文件重新加载\"}";
}

// 物流状态质询
std::string WebGui::handleApiStatus(const std::map<std::string, std::string>& params) {
    std::string keyword = params.count("q") ? params.at("q") : "";
    if (keyword.empty()) return "{\"found\":false}";

    Goods* target = nullptr;
    int scheme = 3;

    // 遍历所有方案寻找货物
    for(int s=1; s<=4; ++s) {
        for(auto& g : logisticsTree[s].goodsList) {
            // 支持按ID或名称查询
            if(g.name == keyword || std::to_string(g.id) == keyword) {
                target = &g;
                scheme = s;
                break;
            }
        }
        if(target) break;
    }

    if (!target) return "{\"found\":false, \"msg\":\"未找到该货物，可能已发货或不存在\"}";

    // 动态计算路径 (Exp 6 requirement: 节点信息变化后，动态计算)
    // 这里我们直接调用 Dijkstra，它会实时读取 csv 文件，所以只要 csv 变了，路径就算的最新的
    int u = -1, v = -1;
    auto it1 = std::find(belongingRegionList.begin(), belongingRegionList.end(), target->belongingArea);
    auto it2 = std::find(belongingRegionList.begin(), belongingRegionList.end(), target->sendingArea);
    if(it1 != belongingRegionList.end()) u = std::distance(belongingRegionList.begin(), it1);
    if(it2 != belongingRegionList.end()) v = std::distance(belongingRegionList.begin(), it2);

    std::string pathInfo = "无法计算路径";
    if(u!=-1 && v!=-1) {
        pathInfo = Dijkstra::minStep(getGraphForScheme(scheme), u, v, belongingRegionList);
    }
    
    // 转义换行
    std::string escapedPath;
    for (char c : pathInfo) { if(c=='\n') escapedPath+="\\n"; else escapedPath+=c; }

    std::string json = "{";
    json += "\"found\":true,";
    json += "\"id\":" + std::to_string(target->id) + ",";
    json += "\"name\":\"" + target->name + "\",";
    json += "\"status\":\"待发货\","; // 只要还在 list 里就是待发货
    json += "\"scheme\":\"" + std::to_string(scheme) + "\",";
    json += "\"priority\":\"" + Utils::formatDouble(target->priority) + "\",";
    json += "\"currentPath\":\"" + escapedPath + "\"";
    json += "}";
    return json;
}

// 解析 URL 查询字符串
std::map<std::string, std::string> WebGui::parseQuery(const std::string& query) {
    std::map<std::string, std::string> params;
    std::vector<std::string> pairs = Utils::split(query, '&');
    for (const auto& pair : pairs) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = pair.substr(0, eq);
            std::string val = pair.substr(eq + 1);
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

// 根据方案 ID 获取对应的邻接矩阵
std::vector<std::vector<int>> WebGui::getGraphForScheme(int schemeId) {
    int size = belongingRegionList.size();
    if (schemeId == 1) return Utils::readMatrix("../data/regionPrice.csv", size, belongingRegionList);
    if (schemeId == 2) return Utils::readMatrix("../data/regionDistance.csv", size, belongingRegionList);
    if (schemeId == 4) return Utils::readMatrix("../data/regionAir.csv", size, belongingRegionList);
    
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

// 发送 HTTP 响应
void WebGui::sendResponse(int socket, const std::string& content, const std::string& contentType) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += content;
    send(socket, response.c_str(), response.size(), 0);
}