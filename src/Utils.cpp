#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <iostream>

namespace Utils {

    std::vector<std::string> split(const std::string &s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    double daysFromToday(const std::string& dateStr) {
        std::tm tm = {};
        std::istringstream ss(dateStr);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto now = std::chrono::system_clock::now();
        using days = std::chrono::duration<double, std::ratio<86400>>;
        auto diff = std::chrono::duration_cast<days>(now - timePoint);
        return std::ceil(diff.count());
    }

    std::string formatDouble(double value) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << value;
        return ss.str();
    }

    std::vector<std::vector<int>> readMatrix(const std::string& filename, int size, const std::vector<std::string>& allRegions) 
    {
        std::vector<std::vector<int>> matrix(size, std::vector<int>(size, 10000));
        std::ifstream file(filename);
        if (!file.is_open()) return matrix;

        std::string line;
        std::vector<std::string> csvRegions;

        if (std::getline(file, line)) {
             if (!line.empty() && line.back() == '\r') line.pop_back();
             std::vector<std::string> headers = Utils::split(line, ',');
             for (size_t i = 1; i < headers.size(); ++i) {
                 std::string name = headers[i];
                 name.erase(0, name.find_first_not_of(" \t\r\n"));
                 name.erase(name.find_last_not_of(" \t\r\n") + 1);
                 if(!name.empty()) csvRegions.push_back(name);
             }
        }

        std::vector<int> mapCsvToGlobal(csvRegions.size(), -1);
        for(size_t i=0; i<csvRegions.size(); ++i) {
            auto it = std::find(allRegions.begin(), allRegions.end(), csvRegions[i]);
            if (it != allRegions.end()) {
                mapCsvToGlobal[i] = std::distance(allRegions.begin(), it);
            }
        }

        int rowCount = 0;
        while(std::getline(file, line) && rowCount < csvRegions.size()) {
             if (!line.empty() && line.back() == '\r') line.pop_back();
             std::vector<std::string> tokens = Utils::split(line, ',');
             if (tokens.size() > 1) {
                 for (size_t col = 0; col < csvRegions.size(); ++col) {
                     if (col + 1 < tokens.size()) {
                         int u = mapCsvToGlobal[rowCount];
                         int v = mapCsvToGlobal[col];
                         if (u != -1 && v != -1) {
                             try {
                                 std::string val = tokens[col+1];
                                 if(!val.empty()) matrix[u][v] = std::stoi(val);
                             } catch(...) {}
                         }
                     }
                 }
                 rowCount++;
             }
        }
        return matrix;
    }
}