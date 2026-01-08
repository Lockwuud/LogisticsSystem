#include "DBManager.h"
#include "Utils.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <vector>

namespace fs = std::filesystem;

const std::string DATA_DIR = "../data/";

// +转小写，用于不区分大小写的比较
std::string dbToLower(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
    return res;
}

// 去除首尾空格 (防止 " A " 匹配不到 "A")
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// 去除 BOM 头 (防止文件开头的隐藏字符导致第一列列名无法识别)
std::string removeBOM(std::string line) {
    if (line.size() >= 3 && 
        (unsigned char)line[0] == 0xEF && 
        (unsigned char)line[1] == 0xBB && 
        (unsigned char)line[2] == 0xBF) {
        return line.substr(3);
    }
    return line;
}

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

// 辅助函数：重写文件
bool rewriteFile(HANDLE hFile, const std::vector<std::string>& lines) {
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    SetEndOfFile(hFile); // 截断文件

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string line = lines[i] + "\n";
        DWORD bytesWritten;
        if (!WriteFile(hFile, line.c_str(), line.size(), &bytesWritten, NULL)) {
            return false;
        }
    }
    return true;
}

bool DBManager::insertRecord(const std::string& tableName, const std::string& recordLine) {
    std::string path = getFilePath(tableName);
    
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile); return false;
    }

    SetFilePointer(hFile, 0, NULL, FILE_END);
    DWORD bytesWritten;
    std::string line = recordLine + "\n";
    BOOL writeSuccess = WriteFile(hFile, line.c_str(), line.size(), &bytesWritten, NULL);

    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);
    return writeSuccess;
}

std::string DBManager::query(const std::string& tableName, const std::string& key, int columnIndex) {
    std::string path = getFilePath(tableName);
    std::string resultJson = "[";

    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return "[]";

    OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile); return "[]";
    }

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
    std::getline(ss, line); 
    if (!line.empty() && line.back() == '\r') line.pop_back();
    line = removeBOM(line);
    std::vector<std::string> headers = Utils::split(line, ',');
    
    bool first = true;
    std::string keyLower = dbToLower(trim(key));
    while (std::getline(ss, line)) {
        if (line.empty()) continue;
        if (line.back() == '\r') line.pop_back();

        bool match = false;
        std::vector<std::string> cols = Utils::split(line, ',');

        if (key.empty()) {
            match = true;
        } else {
            if (columnIndex >= 0 && columnIndex < (int)cols.size()) {
                if (dbToLower(cols[columnIndex]).find(keyLower) != std::string::npos) match = true;
            } else {
                if (dbToLower(line).find(keyLower) != std::string::npos) match = true;
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

bool DBManager::deleteRecords(const std::string& tableName, const std::string& whereCol, const std::string& whereVal) {
    std::string path = getFilePath(tableName);
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile); return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    std::string fileContent;
    if (fileSize > 0) {
        std::vector<char> buffer(fileSize);
        DWORD bytesRead;
        ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
        fileContent.assign(buffer.data(), bytesRead);
    }

    std::vector<std::string> newLines;
    std::stringstream ss(fileContent);
    std::string line;
    
    if (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        line = removeBOM(line);
        newLines.push_back(line); 
        std::vector<std::string> headers = Utils::split(line, ',');
        
        int targetIdx = -1;
        // [修复] 使用小写比较表头
        std::string whereColLower = dbToLower(whereCol);
        for(size_t i=0; i<headers.size(); ++i) {
            if(dbToLower(trim(headers[i])) == whereColLower) targetIdx = i;
        }

        std::string whereValLower = dbToLower(trim(whereVal));

        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            
            bool shouldDelete = false;
            std::vector<std::string> cols = Utils::split(line, ',');
            
            if (targetIdx != -1 && targetIdx < (int)cols.size()) {
                if (dbToLower(trim(cols[targetIdx])) == whereValLower) shouldDelete = true;
            } else if (whereCol.empty()) {
                shouldDelete = true; 
            }

            if (!shouldDelete) newLines.push_back(line);
        }
    }

    bool success = rewriteFile(hFile, newLines);
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);
    return success;
}

bool DBManager::updateRecords(const std::string& tableName, const std::string& whereCol, const std::string& whereVal, 
                              const std::string& targetCol, const std::string& newVal) {
    std::string path = getFilePath(tableName);
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile); return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    std::string fileContent;
    if (fileSize > 0) {
        std::vector<char> buffer(fileSize);
        DWORD bytesRead;
        ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL);
        fileContent.assign(buffer.data(), bytesRead);
    }

    std::vector<std::string> newLines;
    std::stringstream ss(fileContent);
    std::string line;

    if (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();

        line = removeBOM(line);
        newLines.push_back(line);
        std::vector<std::string> headers = Utils::split(line, ',');
        
        int whereIdx = -1;
        int setIdx = -1;
        
        // [修复] 使用小写比较表头
        std::string whereColLower = dbToLower(whereCol);
        std::string targetColLower = dbToLower(targetCol);

        for(size_t i=0; i<headers.size(); ++i) {
            std::string h = dbToLower(trim(headers[i]));
            if(h == whereColLower) whereIdx = i;
            if(h == targetColLower) setIdx = i;
        }

        std::string whereValLower = dbToLower(trim(whereVal));

        while (std::getline(ss, line)) {
            if (line.empty()) continue;
            if (line.back() == '\r') line.pop_back();
            
            std::vector<std::string> cols = Utils::split(line, ',');
            bool match = false;

            if (whereIdx != -1 && whereIdx < (int)cols.size()) {
                if (dbToLower(trim(cols[whereIdx])) == whereValLower) match = true;
            } else if (whereCol.empty()) {
                match = true; 
            }

            // 只有当找到了 setIdx (列存在) 且行匹配时才更新
            if (match && setIdx != -1 && setIdx < (int)cols.size()) {
                cols[setIdx] = newVal; 
                std::string newLine = "";
                for(size_t i=0; i<cols.size(); ++i) {
                    newLine += cols[i];
                    if(i < cols.size()-1) newLine += ",";
                }
                newLines.push_back(newLine);
            } else {
                newLines.push_back(line);
            }
        }
    }

    bool success = rewriteFile(hFile, newLines);
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);
    return success;
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

int DBManager::getColumnIndex(const std::string& tableName, const std::string& colName) {
    if (colName == "*") return -1;
    
    std::string path = getFilePath(tableName);
    HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) return -2; 

    OVERLAPPED overlapped = {0};
    if (!LockFileEx(hFile, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped)) {
        CloseHandle(hFile); return -2;
    }

    char buffer[1024];
    DWORD bytesRead;
    ReadFile(hFile, buffer, 1024, &bytesRead, NULL);
    
    UnlockFileEx(hFile, 0, 0xFFFFFFFF, 0xFFFFFFFF, &overlapped);
    CloseHandle(hFile);

    std::string content(buffer, bytesRead);
    std::stringstream ss(content);
    std::string headerLine;
    std::getline(ss, headerLine);
    if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();

    headerLine = removeBOM(headerLine);
    auto headers = Utils::split(headerLine, ',');

    // [修复] 使用小写比较
    std::string colNameLower = dbToLower(colName);
    for (size_t i = 0; i < headers.size(); ++i) {
        if (dbToLower(trim(headers[i])) == colNameLower) return i;
    }
    return -2; 
}