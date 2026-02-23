#include "AsyncConnectionPool.h"
#include <chrono>

void AsyncConnectionPool::init(int threadCount) {
    // 初始化物理数据库连接
    for (int i = 0; i < threadCount; ++i) {
        dbConnections.emplace_back();
    }

    // 创建工作线程
    for (int i = 0; i < threadCount; ++i) {
        dbConnections.push_back(std::unique_ptr<Connection>(new Connection()));
    }
    std::cout << "异步连接池初始化完成，线程数: " << threadCount << std::endl;
}

void AsyncConnectionPool::worker() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        Connection* conn = nullptr;
        {
            std::lock_guard<std::mutex> lock(conn_mutex);
            for (auto& c : dbConnections) {
                if (c.get()->isFree) {
                    c.get()->isFree = false;
                    conn = c.get();
                    break;
                }
            }
        }

        if (conn) {
            task(conn);
            {
                std::lock_guard<std::mutex> lock(conn_mutex);
                conn->isFree = true;
            }
        }
    }
}

std::future<void> AsyncConnectionPool::submit(const std::string& sql) {
    auto* task_ptr = new std::packaged_task<void(Connection*)>(
        [sql](Connection* conn) {
            std::cout << "线程 ID: " << std::this_thread::get_id() 
                      << " 正在执行: " << sql << std::endl;
            // 实际执行SQL: mysql_query(conn->mysql, sql.c_str());
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    );

    std::future<void> res = task_ptr->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            delete task_ptr;
            throw std::runtime_error("enqueue on stopped AsyncConnectionPool");
        }
        tasks.emplace([task_ptr](Connection* conn) {
            (*task_ptr)(conn);
            delete task_ptr;
        });
    }
    condition.notify_one();
    return res;
}

AsyncConnectionPool::~AsyncConnectionPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "所有工作线程已停止" << std::endl;
}