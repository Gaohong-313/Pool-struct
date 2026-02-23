#include "ConnectionPool.h"
#include <iostream>

int main() {
    auto pool = ConnectionPool::getInstance();

    std::cout << "场景：提交 A, B, C, D 四个请求，但只有 3 个线程。" << std::endl;

    // 提交 A, B, C (这3个会立刻被分配到3个线程)
    pool->submit("SQL_A: SELECT * FROM table");
    pool->submit("SQL_B: UPDATE table SET ...");
    pool->submit("SQL_C: DELETE FROM table");

    // 稍微等一下，让上面的输出先打印
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 提交 D (此时3个线程都在忙，D 会卡在 submit 函数里寻找空闲连接)
    std::cout << ">>> 提交第四个请求 D ..." << std::endl;
    pool->submit("SQL_D: INSERT INTO table ...");

    // 为了演示效果，主线程挂起久一点，观察输出
    std::this_thread::sleep_for(std::chrono::seconds(10));

    return 0;
}