#ifndef TASK_H
#define TASK_H
#include <string>
#include <mysql/mysql.h>

// 前向声明 Connection 类（避免循环包含）
class Connection;

// 任务结构体：包含一个 SQL 语句和一个数据库连接
struct Task {
    std::string sql;
    Connection* conn; // 指向池中的某个连接

    // 构造函数
    Task(const std::string& s, Connection* c) : sql(s), conn(c) {}
};
#endif