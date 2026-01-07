#include "DBManager.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

// 数据存放目录
const std::string DATA_DIR = "../data/";

std::string DBManager::getFilePath(const std::string& tableName) {
    std::string safeName = tableName;
    // 简单过滤非法字符，防止路径遍历
    if (safeName.find("..") != std::string::npos) safeName = "default";
    return DATA_DIR + safeName + ".csv";
}

bool DBManager::createTable(const std::string& tableName, const std::vector<std::string>& headers) {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }

    std::string path = getFilePath(tableName);
    if (fs::exists(path)) return false; // 表已存在

    std::ofstream file(path);
    if (!file.is_open()) return false;

    // 写入表头
    for (size_t i = 0; i < headers.size(); ++i) {
        file << headers[i];
        if (i < headers.size() - 1) file << ",";
    }
    file << "\n";
    file.close();
    return true;
}

bool DBManager::insertRecord(const std::string& tableName, const std::string& recordLine) {
    std::string path = getFilePath(tableName);
    
    // 打开文件获取句柄
    HANDLE hFile = CreateFileA(
        path.c_str(),
        FILE_APPEND_DATA, // 仅追加权限
        FILE_SHARE_READ,  // 允许别人读（但在我们加锁前）
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED overlapped = {0};
    
    // --- 关键点：申请互斥锁 (LOCKFILE_EXCLUSIVE_LOCK) ---
    // 如果有其他进程正在读或写，这里会阻塞等待，直到获得锁
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile);
        return false;
    }

    // 写入数据
    DWORD bytesWritten;
    std::string line = recordLine + "\n";
    WriteFile(hFile, line.c_str(), line.size(), &bytesWritten, NULL);

    // --- 释放锁 ---
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);

    return true;
}

std::string DBManager::query(const std::string& tableName, const std::string& key, int columnIndex) {
    std::string path = getFilePath(tableName);
    std::string resultJson = "[";

    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, // 允许共享
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return "[]";

    OVERLAPPED overlapped = {0};

    // --- 关键点：申请共享锁 (0) ---
    // 0 代表共享锁。允许多个进程同时进入这里读，但拒绝任何进程写
    if (!LockFileEx(hFile, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile);
        return "[]";
    }

    // 读取全部内容
    DWORD fileSize = GetFileSize(hFile, NULL);
    std::string fileContent;
    if (fileSize > 0) {
        std::vector<char> buffer(fileSize);
        DWORD bytesRead;
        ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
        fileContent.assign(buffer.data(), bytesRead);
    }

    // --- 释放锁 ---
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);

    // 内存解析 CSV
    std::stringstream ss(fileContent);
    std::string line;
    std::getline(ss, line); // 表头
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> headers = Utils::split(line, ',');
    
    bool first = true;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        bool match = false;
        // 如果 key 为空，视为查询所有
        if (key.empty()) {
            match = true;
        } else {
            // 简单包含查询
            if (line.find(key) != std::string::npos) match = true;
        }

        if (match) {
            std::vector<std::string> cols = Utils::split(line, ',');
            if (!first) resultJson += ",";
            resultJson += "{";
            for(size_t i=0; i<cols.size() && i<headers.size(); ++i) {
                resultJson += "\"" + headers[i] + "\":\"" + cols[i] + "\"";
                if(i < cols.size()-1 && i < headers.size()-1) resultJson += ",";
            }
            resultJson += "}";
            first = false;
        }
    }
    resultJson += "]";
    return resultJson;
}

std::vector<std::string> DBManager::getAllTables() {
    std::vector<std::string> tables;
    if (!fs::exists(DATA_DIR)) return tables;

    for (const auto& entry : fs::directory_iterator(DATA_DIR)) {
        if (entry.path().extension() == ".csv") {
            tables.push_back(entry.path().stem().string());
        }
    }
    return tables;
}