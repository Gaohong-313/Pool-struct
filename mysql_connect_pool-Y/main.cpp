#include "AsyncConnectionPool.h"
#include <vector>

int main() {
    auto pool = AsyncConnectionPool::getInstance();
    pool->init(10); // 10个线程

    std::vector<std::future<void>> futures;

    // 提交20个任务
    for (int i = 0; i < 20; ++i) {
        std::string sql = "INSERT INTO test VALUES (" + std::to_string(i) + ")";
        futures.push_back(pool->submit(sql));
    }

    std::cout << "已提交 20 个任务，等待执行完成..." << std::endl;

    // 等待所有任务完成
    for (auto& f : futures) {
        f.wait();
    }

    return 0;
}