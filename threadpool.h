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
// ����������
//�û������Զ��������������ͣ���Task�̳У���дrun������ʵ���Զ�����������
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

//�߳�����
class Thread
{
public:
	//�̺߳�����������
	using ThreadFunc = std::function<void()>;
	//�̹߳���
	Thread(ThreadFunc func);
	~Thread();
	//�����߳�
	void start();
private:
	ThreadFunc func_;
};

// �̳߳�����
class ThreadPool {

public:
	// �̳߳ع���
	ThreadPool();
	~ThreadPool();

	// �����̳߳ع���ģʽ
	void setMode(PoolMode mode);

	// ���ó�ʼ���߳�����
	void setInitThreadSize(size_t size);
	

	// �����̳߳�
	void start(size_t initThreadSize = 4);

	// ��������������޵���ֵ
	void setTaskQueMaxThreadHold(int threadhold);

	//���̳߳��ύ����
	void submitTask(std::shared_ptr<Task> sp);
	

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;
private:
	// �����̺߳���
	void threadFunc();

private:
		std::vector<std::shared_ptr<Thread>> threads_;	//�̳߳��б�
		size_t initThreadSize_;	//��ʼ�߳�����

		std::queue<std::shared_ptr<Task>> taskQueue_;	//�������
		std::atomic_uint taskSize_; 	// ԭ������ ���������
		size_t taskQueMaxThreadHold_;	//��������������޵���ֵ


		std::mutex	taskQueMutex_;	//��֤������е��̰߳�ȫ
		/* 
		* �����������������
		* 1. ���գ��������߳̿�������
		* 2. �������������߳̿�������
		*/
		std::condition_variable notFull_;	//������в���
		std::condition_variable notEmpty_;	//������в���

		PoolMode poolMode_;	//��ǰ�̳߳صĹ���ģʽ
		
};



#endif
