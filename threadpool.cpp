#include"threadpool.h"
#include<functional>
#include<thread>
#include<iostream>
const int TASK_MAX_THREADHOLDS = 1024;

ThreadPool::ThreadPool()
	:initThreadSize_(4),
	taskSize_(0),
	taskQueMaxThreadHold_(TASK_MAX_THREADHOLDS),
	poolMode_(PoolMode::MODE_FIXED)
{}

ThreadPool::~ThreadPool()
{}

void ThreadPool::setMode(PoolMode mode)
{
	poolMode_ = mode;
}

void ThreadPool::setInitThreadSize(size_t size)
{
	initThreadSize_ = size;
}

void ThreadPool::setTaskQueMaxThreadHold(int threadhold)
{
	taskQueMaxThreadHold_ = threadhold;
}

// 给线程池提交任务：用户调用该接口，传入任务对象，生产任务
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//获取互斥锁：生产者
	std::unique_lock<std::mutex> lock(taskQueMutex_);

	// 线程通信：信号量、条件变量，等待任务队列有空余
	//while (taskQueue_.size() == taskQueMaxThreadHold_)
	//{
	//	// 改变线程	 等待 std::condition_variable notFull_; 任务队列不满
	//	notFull_.wait(lock);

	//}
	//不能一直阻塞，加入时间限制，最长不能阻塞超过1s，否则判断提交任务失败
	
	
	if(!notFull_.wait_for(lock, std::chrono::seconds(5), [&]()->bool { return taskQueue_.size() < taskQueMaxThreadHold_; }))
	{
		// 如果notFull_返回值为false，提交任务失败
		std::cerr << "task queue is full, submit task fail..." << std::endl;
		return;
	}
	// 有空余，把任务放入任务队列
	taskQueue_.emplace(sp);
	taskSize_++;
	// 任务队列不空，notEmpty_ 上进行通知,分配线程执行任务
	notEmpty_.notify_all();
}


// 定义线程函数：线程池中的所有线程从任务队列里面消费任务
// 从任务队列取任务
void ThreadPool::threadFunc()
{
	/*std::cout << "begin threadFunc... ,tid: " << std::this_thread::get_id() <<std::endl;
	std::cout << "end threadFunc... ,tid: " << std::this_thread::get_id() << std::endl;*/
	// 循环从线程池取任务
	for (;;)
	{
		std::shared_ptr<Task> task;
		{
		// 获取锁
		std::unique_lock<std::mutex> lock(taskQueMutex_);
		// 等待 notEmpty_

		std::cout << "tid: " << std::this_thread::get_id() << " trying ..." << std::endl;


		notEmpty_.wait(lock, [&]()->bool {return taskQueue_.size() > 0; });

		std::cout << "tid: " << std::this_thread::get_id() << " success..." << std::endl;

		// 取一个任务
		task = taskQueue_.front();
		taskQueue_.pop();
		taskSize_--;

		//如果依然有剩余任务，通知其他线程执行任务
		if (taskQueue_.size() > 0)
		{
			notEmpty_.notify_all();
		}

		// 获得任务，进行通知，通知可以生产线程了
		notFull_.notify_all();
		//释放锁
		}
		
		// 当前线程负责执行
		if (task != nullptr)
		{
			
			task->run();
		}
		
	}

}


//开启线程池
void ThreadPool::start( size_t initThreadSize)
{
	// 记录初始线程个数
	initThreadSize_ = initThreadSize;

	// 创建线程对象的时候，把线程函数传递给thread线程对象
	// bind( );
	for (size_t i = 0; i < initThreadSize_; i++)
	{
		auto ptr = std::make_shared<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(ptr);
	}

	// 启动所有线程
	for (size_t i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();	// 需要执行线程函数，线程函数：1. 有没有任务 2. 执行任务
	 }
	
}


/// <summary>
/// 线程方法实现
/// </summary>

// 线程构造
Thread::Thread(ThreadFunc func)
	:func_(func)
{}
Thread::~Thread()
{

}
// 启动线程
void Thread::start()
{
	//创建一个线程来执行一个线程函数
	std::thread t(func_);	//c++11 t 和线程函数func_
	t.detach();	//设置分离线程
}

