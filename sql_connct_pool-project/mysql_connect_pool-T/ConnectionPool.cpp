#include "ConnectionPool.h"
#include <iostream>
#include <chrono>

ConnectionPool::ConnectionPool() : _stop(false) {
    initConnections(); // 初始化3个连接
    // 创建3个工作线程
    for (int i = 0; i < 3; ++i) {
        _workers.emplace_back(&ConnectionPool::workerThread, this);
    }
}

ConnectionPool::~ConnectionPool() {
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _stop = true;
    }
    _cond.notify_all();
    for (auto& w : _workers) {
        if (w.joinable()) w.join();
    }
}

void ConnectionPool::initConnections() {
    // 创建3个连接
    for (int i = 0; i < 3; ++i) {
        Connection conn;
        // 这里省略 mysql_real_connect 的具体参数设置...
        conn.inUse = false; // 初始状态：空闲
        _connections.push_back(std::unique_ptr<Connection>(new Connection()));
    }
    std::cout << "初始化了 3 个数据库连接。" << std::endl;
}

// 用户调用这个函数提交请求
void ConnectionPool::submit(const std::string& sql) {
    std::unique_lock<std::mutex> lock(_mutex);

    // --- 关键逻辑：寻找空闲连接（模拟你描述的“循环访问队列”）---
    Connection* targetConn = nullptr;
    while (true) {
        for (auto& conn : _connections) {
            if (!conn.get()->inUse) { // 找到空闲线程/连接
                conn.get()->inUse = true; // 上锁！占用它
                targetConn = conn.get();
                break;
            }
        }
        if (targetConn) break; // 找到了，跳出等待循环
        else {
            // 没找到（全忙），等待一下再找（或者这里可以加一个短暂的sleep，避免死循环占用CPU）
            lock.unlock();
            std::this_thread::yield(); // 礼貌地让出CPU
            lock.lock();
        }
    }

    // 将任务放入队列
    _taskQueue.push(Task(sql, targetConn));
    std::cout << "提交任务: " << sql << " (等待线程处理...)" << std::endl;
    _cond.notify_one(); // 通知一个工作线程来取任务
}

// 工作线程函数（每个线程都在跑这个逻辑）
void ConnectionPool::workerThread() {
    while (true) {
        Task task("", nullptr);

        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 等待条件变量：有任务进来 或 停止
            _cond.wait(lock, [this] { return !_taskQueue.empty() || _stop; });

            if (_stop && _taskQueue.empty()) break;

            // 取出任务
            task = _taskQueue.front();
            _taskQueue.pop();
        }

        // --- 离开锁的范围，在这里执行耗时的SQL操作 ---
        if (task.conn && task.conn->conn) {
            std::cout << "线程 [" << std::this_thread::get_id() 
                      << "] 正在处理: " << task.sql << std::endl;
            
            // 模拟执行SQL（实际调用 mysql_query）
            // mysql_query(task.conn->conn, task.sql.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(2)); // 模拟耗时

            std::cout << "线程 [" << std::this_thread::get_id() 
                      << "] 完成: " << task.sql << std::endl;

            // 关键：任务完成后，释放锁（标记连接为空闲）
            {
                std::lock_guard<std::mutex> lock(_mutex);
                task.conn->inUse = false; // 解锁！归还连接
            }
        }
    }
}