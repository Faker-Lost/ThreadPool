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
	// ʵ�����������
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
		//����������޸��߳���������ڴ�
		m_threadIDs = new pthread_t[maxNum];

		if (m_threadIDs == nullptr)
		{
			cout << "malloc threadIDs fail..." << endl;
			break;
		}
		// ��ʼ������
		memset(m_threadIDs, 0, sizeof(m_threadIDs) * maxNum);
		// ��ʼ��������
		if (pthread_mutex_init(&m_lock, NULL) != 0 ||
			pthread_cond_init(&m_notEmpty, NULL) != 0)
		{
			cout << "init mutex or condition fail..." << endl;
			break;
		}
		m_shutdown = false;
		 
		//�����̣߳������ߡ��������߳�
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
//��������:������Դ
template<typename T>
ThreadPool<T>::~ThreadPool()
{
	m_shutdown = 1;
	//���ٹ������߳�
	pthread_join(m_managerID, NULL);
	//���������������߳�
	for (int i = 0; i < m_aliveNum; i++)
	{
		pthread_cond_signal(&m_notEmpty);
	}
	//�ͷŶ��ڴ�
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
	// ������񣬵���TaskQueue�е�addTask��Ա����
	m_taskQ->addTask(task);

	//�������������߳�
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
	ThreadPool* pool = static_cast<ThreadPool*>(arg);	//����ת��

	while (true)
	{
		//����������м���
		pthread_mutex_lock(&pool->m_lock);
		//�ж���������Ƿ�Ϊ��,���Ϊ������
		while (pool->m_taskQ->getTaskNum() == 0 && !pool->m_shutdown)
		{
			cout << "Thread " << to_string(pthread_self()) << "   waiting..." << endl;
			
			pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);
			//����������ж��Ƿ�Ҫ�����߳�
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
		//�ж��̳߳��Ƿ�ر�
		if (pool->m_shutdown)
		{
			pthread_mutex_unlock(&pool->m_lock);
			pool->threadExit();
		}

		// �����������ȡ��һ������
		Task<T> task = pool->m_taskQ->takeTask();
		// �������߳� + 1
		pool->m_busyNum++;
		pthread_mutex_unlock(&pool->m_lock);
		//ִ������
		cout << "Thread " << to_string(pthread_self()) << "++++++++start working++++++++" << endl;
		//bug���� void* ���Ͳ���ȫ�����ͷ�
		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;

		//���������
		cout << "Thread " << to_string(pthread_self()) << "---------end working---------" << endl;
		pthread_mutex_lock(&pool->m_lock);
		pool->m_busyNum--;
		pthread_mutex_unlock(&pool->m_lock);

	}
	return nullptr;
}
//�������߳�������
template<typename T>
void* ThreadPool<T>::manager(void* arg)
{
	//����ת��
	ThreadPool* pool = static_cast<ThreadPool*> (arg);

	//�̳߳�û�йرգ�һֱ���
	while (!pool->m_shutdown)
	{
		sleep(5);

		//ȡ���̳߳��е����������߳�����
		pthread_mutex_lock(&pool->m_lock);
		int queueSize = pool->m_taskQ->getTaskNum();
		int liveNum = pool->m_aliveNum;
		int busyNum = pool->m_busyNum;
		pthread_mutex_unlock(&pool->m_lock);

		//�����߳�
		const int NUMBER = 2;
		if (queueSize > liveNum && liveNum < pool->m_maxNum)
		{
			// �̳߳ؼ���
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

		//�����߳�
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
	//��ȡ��ǰ�߳�id
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
