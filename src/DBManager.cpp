#include "DBManager.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace fs = std::filesystem;

const std::string DATA_DIR = "../data/";

std::string DBManager::getFilePath(const std::string& tableName) {
    std::string safeName = tableName;
    if (safeName.find("..") != std::string::npos) safeName = "default";
    return DATA_DIR + safeName + ".csv";
}

bool DBManager::createTable(const std::string& tableName, const std::vector<std::string>& headers) {
    if (!fs::exists(DATA_DIR)) {
        fs::create_directory(DATA_DIR);
    }

    std::string path = getFilePath(tableName);
    if (fs::exists(path)) return false; 

    std::ofstream file(path);
    if (!file.is_open()) return false;

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
    
    // [修复] 使用 GENERIC_WRITE 替代 FILE_APPEND_DATA，否则 LockFileEx 会因权限不足失败
    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ, 
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cerr << "[Error] 打开文件失败: " << path << " Error: " << GetLastError() << std::endl;
        return false;
    }

    OVERLAPPED overlapped = {0};
    
    // 申请互斥锁
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        std::cerr << "[Error] 加锁失败 Error: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return false;
    }

    // [修复] 手动移动指针到文件末尾以实现追加
    SetFilePointer(hFile, 0, NULL, FILE_END);

    DWORD bytesWritten;
    std::string line = recordLine + "\n";
    BOOL writeSuccess = WriteFile(hFile, line.c_str(), line.size(), &bytesWritten, NULL);

    // 释放锁
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);

    if (!writeSuccess) {
        std::cerr << "[Error] 写入数据失败 Error: " << GetLastError() << std::endl;
        return false;
    }

    return true;
}

std::string DBManager::query(const std::string& tableName, const std::string& key, int columnIndex) {
    std::string path = getFilePath(tableName);
    std::string resultJson = "[";

    HANDLE hFile = CreateFileA(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE, 
        FILE_SHARE_READ | FILE_SHARE_WRITE, 
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) return "[]";

    OVERLAPPED overlapped = {0};

    // 申请共享锁
    if (!LockFileEx(hFile, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile);
        return "[]";
    }

    // 读取内容
    DWORD fileSize = GetFileSize(hFile, NULL);
    std::string fileContent;
    if (fileSize > 0) {
        std::vector<char> buffer(fileSize);
        DWORD bytesRead;
        ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
        fileContent.assign(buffer.data(), bytesRead);
    }

    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);

    std::stringstream ss(fileContent);
    std::string line;
    
    // 读取表头
    std::getline(ss, line); 
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::vector<std::string> headers = Utils::split(line, ',');
    
    bool first = true;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        bool match = false;
        // 必须先分割，因为如果是指定列查询，需要访问 cols[index]
        std::vector<std::string> cols = Utils::split(line, ',');

        if (key.empty()) {
            match = true;
        } else {
            // [核心修改] 判断是指定列查询还是全文查询
            if (columnIndex >= 0 && columnIndex < (int)cols.size()) {
                // 指定列查询：只检查对应列是否包含关键字
                if (cols[columnIndex].find(key) != std::string::npos) match = true;
            } else {
                // 全文查询：检查整行
                if (line.find(key) != std::string::npos) match = true;
            }
        }

        if (match) {
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