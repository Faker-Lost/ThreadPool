#pragma once
#include"TaskQueue.h"
using namespace std;
template<typename T>
class ThreadPool
{
public:

    ThreadPool<T>(int min, int max);
    ~ThreadPool<T>();

    // 添加任务 给任务队列添加任务
    void addTask(Task<T> task);
    // 获取忙线程的个数
    int getBusyNumber();
    // 获取活着的线程个数
    int getAliveNumber();

private:

    // 工作的线程的任务函数
    static void* worker(void* arg);
    // 管理者线程的任务函数
    static void* manager(void* arg);
    // 单个线程退出
    void threadExit();

private:  
    // 任务队列
    TaskQueue<T>* m_taskQ;

    pthread_mutex_t m_lock;
    pthread_cond_t m_notEmpty;          //条件变量

    pthread_t* m_threadIDs;
    pthread_t m_managerID;

    int m_minNum;
    int m_maxNum;
    int m_busyNum;
    int m_aliveNum;
    int m_exitNum;
    bool m_shutdown = false;
};

