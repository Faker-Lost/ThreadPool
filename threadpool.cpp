#include"threadpool.h"
#include<functional>
#include<thread>
#include<iostream>

const int TASK_MAX_THREADHOLDS = 1024;
const int THREAD_MAX_THREADHOLDS = 10;

// �̳߳ع���
ThreadPool::ThreadPool()
	:initThreadSize_(4),
	taskSize_(0),
	taskQueMaxThreadHold_(TASK_MAX_THREADHOLDS),
	poolMode_(PoolMode::MODE_FIXED),
	isPoolRunning_(false),
	idleThreads_(0),
	threadSizeMaxHold_(),
	curThreadSize_(0)
{}

ThreadPool::~ThreadPool()
{}

// �̳߳ع���ģʽ
void ThreadPool::setMode(PoolMode mode)
{
	if (checkRunning())
	{
		return;
	}
	poolMode_ = mode;
}

void ThreadPool::setInitThreadSize(size_t size)
{
	if (checkRunning())
	{
		return;
	}
	initThreadSize_ = size;
}

// ����task�������������ֵ
void ThreadPool::setTaskQueMaxThreadHold(int threadhold)
{
	if (checkRunning())
	{
		return;
	}
	taskQueMaxThreadHold_ = threadhold;
}

void ThreadPool::setThreadSizeMaxHold(size_t maxThreadHold)
{
	if (checkRunning())
	{
		return;
	}
	if (poolMode_ == PoolMode::MODE_CACHED)
	{
		threadSizeMaxHold_ = maxThreadHold;
	}
}

// ���̳߳��ύ�����û����øýӿڣ��������������������
Result ThreadPool::submitTask(std::shared_ptr<Task> sp)
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


	if (!notFull_.wait_for(lock, std::chrono::seconds(5), [&]()->bool { return taskQueue_.size() < taskQueMaxThreadHold_; }))
	{
		// ���notFull_����ֵΪfalse���ύ����ʧ��
		std::cerr << "task queue is full, submit task fail..." << std::endl;
		return	Result(sp, false);
	}
	// �п��࣬����������������
	taskQueue_.emplace(sp);
	taskSize_++;
	// ������в��գ�notEmpty_ �Ͻ���֪ͨ,�����߳�ִ������
	notEmpty_.notify_all();

	//cachedģʽ ��������ȽϿ��С  ��Ҫ�������������Ϳ����̵߳��������ж��Ƿ񴴽��µ��߳�
	if (poolMode_ == PoolMode::MODE_CACHED 
		&& taskSize_>idleThreads_
		&& curThreadSize_ < threadSizeMaxHold_)
	{
		// �������߳�
		auto ptr = std::make_shared<Thread>(std::bind(&ThreadPool::threadFunc, this));
		threads_.emplace_back(ptr);
	}

	// ���������Result����
	return Result(sp);
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

			idleThreads_--;

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

			//task->run();	//ִ�����񣬰�����ķ���ֵͨ��setVal��������
			task->exec();
		}

		idleThreads_++;
	}

}

bool ThreadPool::checkRunning() const
{
	return isPoolRunning_;
}


//�����̳߳�
void ThreadPool::start(size_t initThreadSize)
{
	// �����̳߳ص�����״̬
	isPoolRunning_ = true;
	// ��¼��ʼ�̸߳���
	initThreadSize_ = initThreadSize;
	curThreadSize_ = initThreadSize;

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
		idleThreads_++;			// ��¼��ʼ�����̸߳���
	}

}



///////////////////////////////////////////////// �̷߳���ʵ��

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

int Thread::getId() const
{
	return 0;
}

////////////////////////////////////////////  Result ������ʵ��   
Result::Result(std::shared_ptr<Task> task, bool isValid)
	:isValid_(isValid),
	task_(task)
{
	task_->setResult(this);
}

void Result::setVal(Any any)
{
	// �洢task�ķ���ֵ
	this->any_ = std::move(any);
	sem_.post();
}
// �û�����get()
Any Result::get()
{
	if (!isValid_)
	{
		return " ";
	}
	// Task����Ϊִ���꣬�����̱߳�����
	sem_.wait();
	return std::move(any_);
}

//////////// Taskʵ�ַ��� ��װ
Task::Task()
	:result_(nullptr)
{}

void Task::exec()
{
	if (result_ != nullptr)
	{
		result_->setVal(run());		//��̬����
	}
}

void Task::setResult(Result* res)
{
	result_ = res;
}

