#pragma once
#include<pthread.h>
#include<queue>
using namespace std;

//定义任务结构体
using callback = void(*)(void*);
template<typename T>
struct Task
{
	Task<T>()
	{
		arg = nullptr;
		function = nullptr;
	}
	Task<T>(callback f, void* arg)
	{
		function = f;
		this->arg = (T*)arg;
	}
	callback function ;
	T* arg;
};

//任务队列
template<typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	//添加任务 任务队列 需要线程同步
	void addTask(Task<T>& task);
	void addTask(callback func, void* arg);
	//取出任务
	Task<T> takeTask();
	// 获取当前队列任务个数
	size_t getTaskNum()
	{
		return m_queue.size();
	}

private:
	pthread_mutex_t m_mutex;//互斥锁
	queue<Task<T>> m_queue;	//任务队列
};

