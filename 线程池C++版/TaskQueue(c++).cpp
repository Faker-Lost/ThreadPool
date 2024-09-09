#include "TaskQueue.h"
#include<sys/socket.h>
template<typename T>
TaskQueue<T>::TaskQueue()
{
	//��ʼ��������
	pthread_mutex_init(&m_mutex, NULL);
}
template<typename T>
TaskQueue<T>::~TaskQueue()
{
	//���ٻ�����
	pthread_mutex_destroy(&m_mutex);
}
template<typename T>
void TaskQueue<T>::addTask(Task<T>& task)
{
	//����
	pthread_mutex_lock(&m_mutex);
	m_queue.emplace(task);
	//����
	pthread_mutex_unlock(&m_mutex);
}
//���غ���
template<typename T>
void TaskQueue<T>::addTask(callback func, void* arg)
{
	//����
	pthread_mutex_lock(&m_mutex);
	//��װ func �� arg
	m_queue.emplace(Task<T>(func, arg));
	//����
	pthread_mutex_unlock(&m_mutex);
}
template<typename T>
Task<T> TaskQueue<T>::takeTask()
{	
	pthread_mutex_lock(&m_mutex);
	Task<T> t;
	//�ж���������Ƿ�Ϊ��
	if (!m_queue.empty())
	{
		t = m_queue.front();
		m_queue.pop();
	}
	pthread_mutex_unlock(&m_mutex);
	return t;
}
