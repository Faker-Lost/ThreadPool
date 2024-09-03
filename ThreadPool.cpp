#include"ThreadPool.h"
#include<iostream>
using namespace std;

ThreadPool::ThreadPool(int min, int max) :m_maxThread(max), m_minThread(min),
m_stop(false), m_freeThread(max), m_curThread(max)
{
    cout << "max : " << max << endl;
    //创建管理者线程 参数：任务函数、可调用对象
    m_manager = new thread(&ThreadPool::manager, this);
    //创建工作线程
    for (int i = 0; i < max; i++)
    {
        // 指定任务函数
        thread t(&ThreadPool::worker, this);
        // 把线程对象放在容器当中
        m_workers.insert(make_pair(t.get_id(), move(t)));
    }
}

//析构函数 释放资源
ThreadPool::~ThreadPool()
{
    m_stop = true;
    //唤醒所有阻塞的线程
    m_condition.notify_all();
    for (auto& it : m_workers) //引用？线程对象不允许拷贝
    {
        thread& t = it.second;
        //判断当前对象是否可连接
        if (t.joinable())
        {
            cout << "******** 线程 " << t.get_id() << " 将要退出..." << endl;
            t.join();
        }
    }
    if (m_manager->joinable())
    {
        m_manager->join();
    }

    delete m_manager;
}

//任务队列 共享资源，线程从任务队列中取资源，需要进行线程同步
void ThreadPool::addTask(function<void(void)> task)
{
    {
        //管理互斥锁类
        lock_guard<mutex> locker(m_queueMutex);
        m_task.emplace(task);
    }
    //唤醒函数
    m_condition.notify_one();
}

//管理者线程
void ThreadPool::manager(void)
{
    // 检测工作者线程的数量，控制增加和减少
    while (!m_stop.load())
    {
        //睡眠3秒
        this_thread::sleep_for(chrono::seconds(1));
        //读取空闲的线程和线程池里的线程
        int freeidel = m_freeThread.load();
        int cur = m_curThread.load();
        //线程的删除
        if (freeidel > cur / 2 && cur > m_minThread)
        {
            //销毁线程,每次销毁两个线程
            m_deleteThread.store(2);
            m_condition.notify_all();
            //m_ids 为共享资源
            lock_guard<mutex> ids_lock(m_idsMutex);
            for (auto id : m_ids)
            {
                auto it = m_workers.find(id);
                if (it != m_workers.end())
                {
                    cout << "======== 线程： " << (*it).first << "被销毁..." << endl;
                    (*it).second.join();
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        }
        //线程的添加
        else if (freeidel == 0 && cur < m_maxThread)
        {
            // 把线程对象放在容器当中
            // 指定任务函数
            thread t(&ThreadPool::worker, this);
            cout << "+++++++++添加一个线程，id： " << t.get_id() << endl;
            // 把线程对象放在容器当中
            m_workers.insert(make_pair(t.get_id(), move(t)));
            m_curThread++;
            m_freeThread++;
        }
    }
}

//工作者线程
void ThreadPool::worker(void)
{
    //判断任务队列是否为空 空：阻塞
    while (!m_stop.load())
    {
        function<void(void)> task = nullptr;

        {
            unique_lock<mutex> locker(m_queueMutex);

            //判断任务队列是否为空&& 是否打开线程池
            while (m_task.empty() && !m_stop)
            {
                //阻塞当前线程
                m_condition.wait(locker);
                if (m_deleteThread.load() > 0)
                {
                    cout << "-----------线程退出，ID：" << this_thread::get_id() << endl;
                    m_curThread--;
                    m_freeThread--;
                    m_deleteThread--;
                    //线程退出并保存退出线程id
                    //m_ids 共享资源，需要加锁
                    lock_guard<mutex> ids_lock(m_idsMutex);
                    m_ids.emplace_back(this_thread::get_id());
                    return;
                }
            }

            //取任务
            if (!m_task.empty())
            {
                //避免拷贝
                cout << "取出一个任务..." << endl;
                task = move(m_task.front());
                //弹出
                m_task.pop();
            }
        }

        //执行任务
        if (task)
        {
            m_freeThread--;
            task();
            m_freeThread++;
        }
    }
}
//阻塞等待
void calc(int x, int y)
{
    int z = x + y;
    cout << "z = " << z << endl;
    this_thread::sleep_for(chrono::seconds(5));
}
//得到一个返回值，返回一个整型
int calc1(int x, int y)
{
    int z = x + y;
    this_thread::sleep_for(chrono::seconds(5));
    return z;
}


int main()
{
    ThreadPool pool;
    vector<future<int>> result;
    for (int i = 0; i < 10; i++)
    {
        //auto obj = bind(calc, i, i * 2);
        result.emplace_back(pool.addTask(calc1, i, i*2));
    }
    for (auto& item : result)
    {
        cout << "线程执行的结构：" << item.get() << endl;
    }
    //保证主线程不退出
    getchar();


    return 0;
}