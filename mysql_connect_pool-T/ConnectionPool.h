#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include "Task.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <memory>

// 假设连接类（简化版，只关注连接句柄）
class Connection {
public:

    MYSQL* conn;
    bool inUse;

    Connection() : inUse(false) {
        conn = mysql_init(nullptr);
        // 这里可以初始化连接...
    }
    ~Connection() {
        if (conn) mysql_close(conn);
    }
  
};

class ConnectionPool {
private:
    // 单例构造
    ConnectionPool();
    ~ConnectionPool();

public:
    // 获取单例实例
    static ConnectionPool* getInstance() {
        static ConnectionPool pool;
        return &pool;
    }

    // 用户调用的接口：提交 SQL 请求
    void submit(const std::string& sql);

private:
    // 工作线程函数：循环获取任务并执行
    void workerThread();

    // 初始化连接池（创建3个连接）
    void initConnections();

private:
    // 1. 固定大小的线程队列（std::vector 存储线程）
    std::vector<std::thread> _workers;
    
    // 2. 任务队列（缓冲区）
    std::queue<Task> _taskQueue;
    
    // 3. 保护任务队列的互斥锁
    std::mutex _mutex;
    
    // 4. 条件变量，用于通知线程有新任务来了
    std::condition_variable _cond;
    
    // 5. 连接数组（3个连接）
    std::vector<std::unique_ptr<Connection>> _connections;
    
    // 6. 停止标志
    std::atomic<bool> _stop;
};

#endif