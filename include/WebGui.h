#ifndef WEB_GUI_H
#define WEB_GUI_H

#include <string>

class WebGui {
public:
    // 启动 Web 服务器 (会阻塞，直到用户在网页端选择关闭或手动停止)
    void start();

private:
    // 处理单个客户端请求
    void handleClient(int clientSocket);
    
    // 生成主页 HTML (包含所有表格)
    std::string generateDashboard();
    
    // 处理发货请求 (删除物品)
    void handleShipRequest(const std::string& request);
    
    // 简单的 URL 参数解析辅助函数
    int getParam(const std::string& url, const std::string& key);
};

#endif // WEB_GUI_H