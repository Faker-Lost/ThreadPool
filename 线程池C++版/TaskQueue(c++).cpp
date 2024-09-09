#include "TaskQueue.h"
#include<sys/socket.h>
template<typename T>
TaskQueue<T>::TaskQueue()
{
	//初始化互斥锁
	pthread_mutex_init(&m_mutex, NULL);
}
template<typename T>
TaskQueue<T>::~TaskQueue()
{
	//销毁互斥锁
	pthread_mutex_destroy(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(Task<T>& task)
{
	//加锁
	pthread_mutex_lock(&m_mutex);
	m_queue.emplace(task);
	//解锁
	pthread_mutex_unlock(&m_mutex);
}
//重载函数
template<typename T>
void TaskQueue<T>::addTask(callback func, void* arg)
{
	//加锁
	pthread_mutex_lock(&m_mutex);
	//包装 func 和 arg
	m_queue.emplace(Task<T>(func, arg));
	//解锁
	pthread_mutex_unlock(&m_mutex);
}
template<typename T>
Task<T> TaskQueue<T>::takeTask()
{	
	pthread_mutex_lock(&m_mutex);
	Task<T> t;
	//判断任务队列是否为空
	if (!m_queue.empty())
	{
		t = m_queue.front();
		m_queue.pop();
	}
	pthread_mutex_unlock(&m_mutex);
	return t;
}
