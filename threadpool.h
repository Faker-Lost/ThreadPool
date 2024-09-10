#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<vector>
#include<queue>
#include<memory>
#include<atomic>
#include<mutex>
#include<condition_variable>
#include <cstddef>
#include<functional>
#include<unordered_map>
#include<iostream>

// Any类：可以接受任意数据的类型
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// 构造函数接受Any类型其他的数据
	template<typename T>
	Any(T data) :base_ptr(std::make_unique<Deriver<T>>(data))
	{}

	// 这个方法能把Any对象里面存储的data数据提取出来
	template<typename T>
	T cast_()
	{
		// 基类指针转成 派生类指针
		Deriver<T>* ptr = dynamic_cast<Deriver<T>*>(base_ptr.get());
		if (ptr == nullptr)
		{
			std::cout << "ptr is nullptr!" << std::endl;
		}
		return ptr->data_;
	}

private:
	// 基类类型
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	template<typename T>
	class Deriver :public Base
	{
	public:
		Deriver(T data) :data_(data)
		{}
		T data_;
	};

private:
	// 定义基类指针
	std::unique_ptr<Base> base_ptr;
};


// 实现信号量类: 1.互斥锁 + 信号量
class Semaphore
{
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{}
	~Semaphore() = default;

	// 获取一个信号量资源
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]()->bool { return resLimit_ > 0; });
		resLimit_--;
	}

	// 增加一个信号量资源
	void post()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	//互斥锁
	std::mutex mtx_;
	// 条件变量
	std::condition_variable cond_;
};

// 实现接受提交到线程池的Task任务执行完成后返回值Result
class Task;		//task前置声明

class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// setVal 获取任务执行完的返回值
	void setVal(Any any);
	// get()，获取task返回值
	Any get();

private:
	Any any_;	//存储任意类型的返回值
	Semaphore sem_;		//信号量
	std::shared_ptr<Task> task_;	//指向对应获取返回值的任务对象
	std::atomic_bool isValid_;		//返回值是否有效	
};


// 任务抽象基类
//用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务类型
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	// 接受任意类型的返回值
	virtual Any run() = 0;

private:
	Result* result_;
};

enum class PoolMode
{
	MODE_FIXED,
	MODE_CACHED,
};

//线程类型
class Thread
{
public:
	//线程函数对象类型
	using ThreadFunc = std::function<void(int)>;
	//线程构造
	Thread(ThreadFunc func);
	~Thread();
	//启动线程
	void start();

	// 获取线程id
	int getId() const;
private:
	ThreadFunc func_;
	static int generateID_;
	int threadID_;		// 保存线程ID
};

// 线程池类型
class ThreadPool {

public:
	// 线程池构造
	ThreadPool();
	~ThreadPool();

	// 设置线程池工作模式
	void setMode(PoolMode mode);

	// 设置初始的线程数量
	void setInitThreadSize(size_t size);

	// 设置cached模式下线程的阈值
	void setThreadSizeMaxHold(size_t threadSizeMaxHold_);

	// 开启线程池 当前系统CPU核心数量
	void start(int  initThreadSize = std::thread::hardware_concurrency());

	// 设置任务队列上限的阈值
	void setTaskQueMaxThreadHold(int threadhold);

	//给线程池提交任务
	Result submitTask(std::shared_ptr<Task> sp);


	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;
private:
	// 定义线程函数
	void threadFunc(int threadid);

	// 检查当前线程启动状态
	bool checkRunning() const;

private:
	std::unordered_map<int, std::unique_ptr<Thread> > threads_;	// 线程列表

	size_t initThreadSize_;							// 初始线程数量
	size_t threadSizeMaxHold_;						// 线程数量上限
	std::atomic_int curThreadSize_;					// 记录当前线程里面线程的数量
	std::atomic_int idleThreadSize_;				// 记录空闲线程数量

	std::queue<std::shared_ptr<Task>> taskQueue_;	//任务队列
	std::atomic_int taskSize_; 						//原子类型 任务的数量
	size_t taskQueMaxThreadHold_;					//任务队列数量上限的阈值
	
	/*
	* 任务队列条件变量：
	* 1. 不空：消费者线程可以消费
	* 2. 不满：生产者线程可以生产
	*/
	std::mutex	taskQueMutex_;						//保证任务队列的线程安全
	std::condition_variable notFull_;				//任务队列不满
	std::condition_variable notEmpty_;				//任务队列不空
	std::condition_variable exitCond_;				// 等待线程资源全部回收

	PoolMode poolMode_;								//当前线程池的工作模式
	std::atomic_bool isPoolRunning_;				// 表示当前线程池的启动状态


};



#endif
