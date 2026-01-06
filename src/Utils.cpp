#include "Utils.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cmath>
#include <chrono>

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
        // 简单实现：解析 yyyy-MM-dd
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
}