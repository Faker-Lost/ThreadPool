#include "ThreadPool.h"
#include"TaskQueue.h"
#include<iostream>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
using namespace std;

template<typename T>
ThreadPool<T>::ThreadPool(int minNum, int maxNum)
{
	// 实例化任务队列
	do
	{
		m_taskQ = new TaskQueue<T>;
		if (m_taskQ == nullptr)
		{
			cout << "malloc taskQ fail ..." << endl;
			break;
		}

		m_maxNum = maxNum;
		m_minNum = minNum;
		m_busyNum = 0;
		m_aliveNum = minNum;
		//根据最大上限给线程数组分配内存
		m_threadIDs = new pthread_t[maxNum];

		if (m_threadIDs == nullptr)
		{
			cout << "malloc threadIDs fail..." << endl;
			break;
		}
		// 初始化数组
		memset(m_threadIDs, 0, sizeof(m_threadIDs) * maxNum);
		// 初始化互斥锁
		if (pthread_mutex_init(&m_lock, NULL) != 0 ||
			pthread_cond_init(&m_notEmpty, NULL) != 0)
		{
			cout << "init mutex or condition fail..." << endl;
			break;
		}
		m_shutdown = false;
		 
		//创建线程：管理者、工作者线程
		pthread_create(&m_managerID, NULL, manager, this);

		for (int i = 0; i < minNum; i++)
		{
			pthread_create(&m_threadIDs[i], NULL, worker, this);
		}
		return;

	} while (1);

	if (m_threadIDs) delete[]m_threadIDs;
	if (m_taskQ) delete m_taskQ;

}
//析构函数:销毁资源
template<typename T>
ThreadPool<T>::~ThreadPool()
{
	m_shutdown = 1;
	//销毁管理者线程
	pthread_join(m_managerID, NULL);
	//唤醒所有消费者线程
	for (int i = 0; i < m_aliveNum; i++)
	{
		pthread_cond_signal(&m_notEmpty);
	}
	//释放堆内存
	if (m_taskQ) delete m_taskQ;
	if (m_threadIDs) delete[] m_threadIDs;

	pthread_mutex_destroy(&m_lock);
	pthread_cond_destroy(&m_notEmpty);
}
template<typename T>
void ThreadPool<T>::addTask(Task<T> task)
{
	if (m_shutdown)
	{
		return;
	}
	// 添加任务，调用TaskQueue中的addTask成员方法
	m_taskQ->addTask(task);

	//唤醒阻塞工作线程
	pthread_cond_signal(&m_notEmpty);
}
template<typename T>
int ThreadPool<T>::getBusyNumber()
{
	int busyNum = 0;
	pthread_mutex_lock(&m_lock);
	busyNum = m_busyNum;
	pthread_mutex_lock(&m_lock);
	return busyNum;
}
template<typename T>
int ThreadPool<T>::getAliveNumber()
{
	int threadNum = 0;
	pthread_mutex_lock(&m_lock);
	threadNum = m_aliveNum;
	pthread_mutex_unlock(&m_lock);
	return threadNum;
}
template<typename T>
void* ThreadPool<T>::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);	//类型转换

	while (true)
	{
		//访问任务队列加锁
		pthread_mutex_lock(&pool->m_lock);
		//判断任务队列是否为空,如果为空阻塞
		while (pool->m_taskQ->getTaskNum() == 0 && !pool->m_shutdown)
		{
			cout << "Thread " << to_string(pthread_self()) << "   waiting..." << endl;
			
			pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);
			//解除阻塞，判断是否要销毁线程
			if (pool->m_exitNum > 0)
			{
				pool->m_exitNum--;
				if (pool->m_aliveNum > pool->m_minNum)
				{
					pool->m_aliveNum--;
					pthread_mutex_unlock(&pool->m_lock);
					pool->threadExit();
				}
			}
		}
		//判断线程池是否关闭
		if (pool->m_shutdown)
		{
			pthread_mutex_unlock(&pool->m_lock);
			pool->threadExit();
		}

		// 从任务队列中取出一个任务
		Task<T> task = pool->m_taskQ->takeTask();
		// 工作的线程 + 1
		pool->m_busyNum++;
		pthread_mutex_unlock(&pool->m_lock);
		//执行任务
		cout << "Thread " << to_string(pthread_self()) << "++++++++start working++++++++" << endl;
		//bug调试 void* 类型不能全部被释放
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		//任务处理结束
		cout << "Thread " << to_string(pthread_self()) << "---------end working---------" << endl;
		pthread_mutex_lock(&pool->m_lock);
		pool->m_busyNum--;
		pthread_mutex_unlock(&pool->m_lock);

	}
	return nullptr;
}
//管理者线程任务函数
template<typename T>
void* ThreadPool<T>::manager(void* arg)
{
	//类型转换
	ThreadPool* pool = static_cast<ThreadPool*> (arg);

	//线程池没有关闭，一直检测
	while (!pool->m_shutdown)
	{
		sleep(5);

		//取出线程池中的任务数和线程数量
		pthread_mutex_lock(&pool->m_lock);
		int queueSize = pool->m_taskQ->getTaskNum();
		int liveNum = pool->m_aliveNum;
		int busyNum = pool->m_busyNum;
		pthread_mutex_unlock(&pool->m_lock);

		//创建线程
		const int NUMBER = 2;
		if (queueSize > liveNum && liveNum < pool->m_maxNum)
		{
			// 线程池加锁
			pthread_mutex_lock(&pool->m_lock);
			int num = 0;
			for (int i = 0; i < pool->m_maxNum && num < NUMBER
				&& pool->m_aliveNum < pool->m_maxNum; ++i)
			{
				if (pool->m_threadIDs[i] == 0)
				{
					pthread_create(&pool->m_threadIDs[i], NULL, worker, pool);
					num++;
					pool->m_aliveNum++;
				}
			}
			pthread_mutex_unlock(&pool->m_lock);
		}

		//销毁线程
		if (busyNum * 2 < liveNum && liveNum > pool->m_minNum)
		{
			pthread_mutex_lock(&pool->m_lock);
			pool->m_exitNum = NUMBER;
			pthread_mutex_unlock(&pool->m_lock);
			for (int i = 0; i < NUMBER; ++i)
			{
				pthread_cond_signal(&pool->m_notEmpty);
			}
		}
	}
	return nullptr;
}
template<typename T>
void ThreadPool<T>::threadExit()
{
	//获取当前线程id
	pthread_t tid = pthread_self();
	for (int i = 0; i < m_maxNum; i++)
	{
		if (m_threadIDs[i] == tid)
		{
			cout << "ThreadExit() function :thread " << to_string(pthread_self()) << " exiting..." << endl;

			m_threadIDs[i] = 0;

			break;
		}
	}
	pthread_exit(NULL);
}
