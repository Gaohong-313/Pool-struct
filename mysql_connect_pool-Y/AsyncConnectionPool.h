#ifndef ASYNC_CONNECTION_POOL_H
#define ASYNC_CONNECTION_POOL_H

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include <future>
#include <mysql/mysql.h>
#include <atomic>

// 将 Connection 类的完整定义放在头文件中
class Connection {
public:
    MYSQL* mysql;
    bool isFree;

    Connection() : mysql(nullptr), isFree(true) {
        mysql = mysql_init(nullptr);
        if (mysql) {
            std::cout << "物理连接创建成功" << std::endl;
        }
    }

    ~Connection() {
        if (mysql) {
            mysql_close(mysql);
        }
    }
};

using Task = std::function<void(Connection*)>;

class AsyncConnectionPool {
public:
    static AsyncConnectionPool* getInstance() {
        static AsyncConnectionPool pool;
        return &pool;
    }

    void init(int threadCount = 10);

    std::future<void> submit(const std::string& sql);

    ~AsyncConnectionPool();

private:
    AsyncConnectionPool() : stop(false) {}

    void worker();

private:
    std::vector<std::thread> workers;
    std::queue<Task> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;

    std::atomic<bool> stop;

    // 现在编译器已经看到了 Connection 的完整定义，可以安全地使用 vector
    std::vector<std::unique_ptr<Connection>> dbConnections;
    std::mutex conn_mutex;
};

#endif