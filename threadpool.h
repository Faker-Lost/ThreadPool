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
#include<iostream>
// 任务抽象基类
//用户可以自定义任意任务类型，从Task继承，重写run方法，实现自定义任务类型
class Task
{
public:
	virtual void run() = 0;
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
	using ThreadFunc = std::function<void()>;
	//线程构造
	Thread(ThreadFunc func);
	~Thread();
	//启动线程
	void start();
private:
	ThreadFunc func_;
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
	

	// 开启线程池
	void start(size_t initThreadSize = 4);

	// 设置任务队列上限的阈值
	void setTaskQueMaxThreadHold(int threadhold);

	//给线程池提交任务
	void submitTask(std::shared_ptr<Task> sp);
	

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;
private:
	// 定义线程函数
	void threadFunc();

private:
		std::vector<std::shared_ptr<Thread>> threads_;	//线程池列表
		size_t initThreadSize_;	//初始线程数量

		std::queue<std::shared_ptr<Task>> taskQueue_;	//任务队列
		std::atomic_uint taskSize_; 	// 原子类型 任务的数量
		size_t taskQueMaxThreadHold_;	//任务队列数量上限的阈值


		std::mutex	taskQueMutex_;	//保证任务队列的线程安全
		/* 
		* 任务队列条件变量：
		* 1. 不空：消费者线程可以消费
		* 2. 不满：生产者线程可以生产
		*/
		std::condition_variable notFull_;	//任务队列不满
		std::condition_variable notEmpty_;	//任务队列不空

		PoolMode poolMode_;	//当前线程池的工作模式
		
};



#endif
