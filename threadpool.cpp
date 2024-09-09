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

// ���̳߳��ύ�����û����øýӿڣ��������������������
void ThreadPool::submitTask(std::shared_ptr<Task> sp)
{
	//��ȡ��������������
	std::unique_lock<std::mutex> lock(taskQueMutex_);

	// �߳�ͨ�ţ��ź����������������ȴ���������п���
	//while (taskQueue_.size() == taskQueMaxThreadHold_)
	//{
	//	// �ı��߳�	 �ȴ� std::condition_variable notFull_; ������в���
	//	notFull_.wait(lock);

	//}
	//����һֱ����������ʱ�����ƣ��������������1s�������ж��ύ����ʧ��
	
	
	if(!notFull_.wait_for(lock, std::chrono::seconds(5), [&]()->bool { return taskQueue_.size() < taskQueMaxThreadHold_; }))
	{
		// ���notFull_����ֵΪfalse���ύ����ʧ��
		std::cerr << "task queue is full, submit task fail..." << std::endl;
		return;
	}
	// �п��࣬����������������
	taskQueue_.emplace(sp);
	taskSize_++;
	// ������в��գ�notEmpty_ �Ͻ���֪ͨ,�����߳�ִ������
	notEmpty_.notify_all();
}


// �����̺߳������̳߳��е������̴߳��������������������
// ���������ȡ����
void ThreadPool::threadFunc()
{
	/*std::cout << "begin threadFunc... ,tid: " << std::this_thread::get_id() <<std::endl;
	std::cout << "end threadFunc... ,tid: " << std::this_thread::get_id() << std::endl;*/
	// ѭ�����̳߳�ȡ����
	for (;;)
	{
		std::shared_ptr<Task> task;
		{
		// ��ȡ��
		std::unique_lock<std::mutex> lock(taskQueMutex_);
		// �ȴ� notEmpty_

		std::cout << "tid: " << std::this_thread::get_id() << " trying ..." << std::endl;


		notEmpty_.wait(lock, [&]()->bool {return taskQueue_.size() > 0; });

		std::cout << "tid: " << std::this_thread::get_id() << " success..." << std::endl;

		// ȡһ������
		task = taskQueue_.front();
		taskQueue_.pop();
		taskSize_--;

		//�����Ȼ��ʣ������֪ͨ�����߳�ִ������
		if (taskQueue_.size() > 0)
		{
			notEmpty_.notify_all();
		}

		// ������񣬽���֪ͨ��֪ͨ���������߳���
		notFull_.notify_all();
		//�ͷ���
		}
		
		// ��ǰ�̸߳���ִ��
		if (task != nullptr)
		{
			
			task->run();
		}
		
	}

}


//�����̳߳�
void ThreadPool::start( size_t initThreadSize)
{
	// ��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;

	// �����̶߳����ʱ�򣬰��̺߳������ݸ�thread�̶߳���
	// bind( );
	for (size_t i = 0; i < initThreadSize_; i++)
	{
		auto ptr = std::make_shared<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(ptr);
	}

	// ���������߳�
	for (size_t i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();	// ��Ҫִ���̺߳������̺߳�����1. ��û������ 2. ִ������
	 }
	
}


/// <summary>
/// �̷߳���ʵ��
/// </summary>

// �̹߳���
Thread::Thread(ThreadFunc func)
	:func_(func)
{}
Thread::~Thread()
{

}
// �����߳�
void Thread::start()
{
	//����һ���߳���ִ��һ���̺߳���
	std::thread t(func_);	//c++11 t ���̺߳���func_
	t.detach();	//���÷����߳�
}

