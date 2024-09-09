#pragma once
#pragma once
#pragma once
#include<thread>
#include<vector>
#include<map>
#include<atomic>
#include<queue>
#include<functional>
#include<mutex>
#include<condition_variable>
#include<future>
#include<memory>
using namespace std;
/* 线程池的构成:
* 1. 管理者线程->子线程 1个
*  - 控制工作线程的数量：增加或减少
* 2. 若干个工作的线程->子线程 n个
*   - 从任务队列中取任务
    - 任务队列为空，被阻塞（被条件变量阻塞）
    - 线程同步（互斥锁）
    - 当前工作线程的数量,空闲的线程数量，
    - 指定最大，最小线程数量
* 3. 任务队列->queue
*   - 互斥锁
*   - 条件变量
* 4. 线程池开关->bool
*/
class ThreadPool
{
public:
    // 1 初始化线程池 hardware_concurrency():读取当前设备CPU核心数
    ThreadPool(int min = 2, int max = thread::hardware_concurrency());
    ~ThreadPool();
    // 任务函数 -> 任务队列
    void addTask(function<void(void)>);
     
    // 需要返回值 指定一个变参,            返回值future类型的对象，类型 typename
    template<typename T, typename...Args>
    // T&& 未定数据类型，根据传入的参数进行推导
    auto addTask(T&& t, Args&&... args) -> future<typename result_of<T(Args...)>::type>
    {
        //将任务函数添加到任务队列
        //1 package_task 类，调用get_future 
        using returnType = typename result_of<T(Args...)>::type;
        //完美转发 保存对应的数据类型
         auto mytask = make_shared< packaged_task<returnType()> > (
                bind(forward<T>(t), forward<Args>(args)...)
            );
        //2 get future()
        future<returnType> res = mytask->get_future();
        //3 package_task 添加到任务队列
        // 匿名对象
        m_queueMutex.lock();
        m_task.emplace(
            [mytask]() {
                (*mytask)();
            });
        m_queueMutex.unlock();
        //唤醒
        m_condition.notify_one( );

        return res;
    }

private:
    void manager(void);
    void worker(void);
private:
    thread* m_manager;          //管理者线程
    map<thread::id, thread> m_workers;
    vector<thread::id> m_ids;   //工作的线程
    atomic<int> m_minThread;    //最小数量
    atomic<int> m_maxThread;    //最大数量
    atomic<int> m_curThread;    //当前线程数量
    atomic<int> m_freeThread;    //空闲线程数量 
    atomic<int> m_deleteThread; //销毁线程
    atomic<bool> m_stop;         //线程池开关
    //函数指针，传递函数指针，被绑定器包装后的可调用对象
    queue<function<void(void)>> m_task;
    //互斥锁
    mutex m_queueMutex;
    mutex m_idsMutex;
    //条件变量， 阻塞消费者线程
    condition_variable m_condition;
};