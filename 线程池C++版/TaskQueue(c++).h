#pragma once
#include<pthread.h>
#include<queue>
using namespace std;

//��������ṹ��
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

//�������
template<typename T>
class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();

	//������� ������� ��Ҫ�߳�ͬ��
	void addTask(Task<T>& task);
	void addTask(callback func, void* arg);
	//ȡ������
	Task<T> takeTask();
	// ��ȡ��ǰ�����������
	size_t getTaskNum()
	{
		return m_queue.size();
	}

private:
	pthread_mutex_t m_mutex;//������
	queue<Task<T>> m_queue;	//�������
};

