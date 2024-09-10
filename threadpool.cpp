#include"threadpool.h"
#include<functional>
#include<thread>
#include<iostream>
const int TASK_MAX_THREADHOLDS = INT32_MAX;
const int THREAD_MAX_THREADHOLDS = 100;
const int THREAD_MAX_IDLE_TIME = 10;	// ��λ s��

// �̳߳ع���
ThreadPool::ThreadPool()
	:initThreadSize_(4),
	taskSize_(0),
	taskQueMaxThreadHold_(TASK_MAX_THREADHOLDS),
	poolMode_(PoolMode::MODE_FIXED),
	isPoolRunning_(false),
	idleThreadSize_(0),
	threadSizeMaxHold_(THREAD_MAX_THREADHOLDS),
	curThreadSize_(0)
{}

// �̳߳�����
ThreadPool::~ThreadPool()
{
	isPoolRunning_ = false;
	
	// �ȴ��̳߳�������̷߳��� 1.������2.����ִ������
	std::unique_lock<std::mutex> lock(taskQueMutex_);
	notEmpty_.notify_all();
	exitCond_.wait(lock, [&]()->bool {return threads_.size()==0; });
}

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
		&& taskSize_ > idleThreadSize_
		&& curThreadSize_ < threadSizeMaxHold_)
	{
		std::cout << "=========Create New Thread=============" << std::endl;
		// �������߳�
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc,this, std::placeholders::_1));
		int threadID = ptr->getId();
		threads_.emplace(threadID, std::move(ptr));
		threads_[threadID]->start();//�����߳�
		// �޸��߳���صı���
		curThreadSize_++;
		idleThreadSize_++;
	}

	// ���������Result����
	return Result(sp);
}


// �����̺߳������̳߳��е������̴߳��������������������
// ���������ȡ����
void ThreadPool::threadFunc(int threadid)		// �̺߳������أ���Ӧ���߳̽���
{

	auto lastTime = std::chrono::high_resolution_clock().now();
	// ѭ�����̳߳�ȡ����
	while(isPoolRunning_)
	{
		std::shared_ptr<Task> task;
		{
			// ��ȡ��
			std::unique_lock<std::mutex> lock(taskQueMutex_);
			// �ȴ� notEmpty_

			std::cout << "Thread tid: " << std::this_thread::get_id() << " trying get work..." << std::endl;

			// ����initThreadSize_,��Ҫ���գ���ǰʱ��-��һ���߳�ִ��ʱ�� > 60s
			if (poolMode_ == PoolMode::MODE_CACHED)
			{
				// ÿһ���ӷ���һ�Σ���ô���ֳ�ʱ���أ����������ִ�з���
				// �� + ˫���ж�
				while (	isPoolRunning_&&taskQueue_.size() == 0)
				{
					//�������� ��ʱ����
					if (poolMode_ == PoolMode::MODE_CACHED)
					{
						if (std::cv_status::timeout == notEmpty_.wait_for(lock, std::chrono::seconds(1)))
						{
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now-lastTime);
							if (dur.count() >= THREAD_MAX_IDLE_TIME
								&& curThreadSize_ > initThreadSize_)
							{
								// ��ʼ�����߳�
								// ��¼�߳�������ֵ�޸�
								// ���̶߳����б��������ɾ��
								// threadid -> �߳�
								threads_.erase(threadid);
								curThreadSize_--;
								idleThreadSize_--;

								std::cout << "Thread tid:" << std::this_thread::get_id() << "exit()" << std::endl;
								return;
							}
						}
					}
					else
					{
						notEmpty_.wait(lock, [&]()->bool {return taskQueue_.size() > 0; });
					}	
				}
				// �̳߳�Ҫ�����������߳���Դ
				/*if (!isPoolRunning_)
				{
					threads_.erase(threadid);
					std::cout << "threadid:" << std::this_thread::get_id() << "exit()" << std::endl;
					exitCond_.notify_all();
					return;
				}*/
				if (!isPoolRunning_)
				{
					break;
				}
			}

			idleThreadSize_--;

			std::cout << "Thread tid: " << std::this_thread::get_id() << " success..." << std::endl;

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

			//task->run();		//ִ�����񣬰�����ķ���ֵͨ��setVal��������
			task->exec();
		}

		idleThreadSize_++;
		lastTime = std::chrono::high_resolution_clock().now();	// �����߳�ִ�������ʱ��
	}

	// ����whileѭ����ɾ���̶߳���
	threads_.erase(threadid);
	std::cout << "Thread tid:" << std::this_thread::get_id() << " exit..." << std::endl;
	exitCond_.notify_all();



}

bool ThreadPool::checkRunning() const
{
	return isPoolRunning_;
}


//�����̳߳�
void ThreadPool::start(int initThreadSize)
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
		auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this,std::placeholders::_1));
		int threadID = ptr->getId();
		threads_.emplace(threadID, std::move(ptr));
	}

	// ���������߳�
	for (int i = 0; i < initThreadSize_; i++)
	{
		threads_[i]->start();	// ��Ҫִ���̺߳������̺߳�����1. ��û������ 2. ִ������
		idleThreadSize_++;			// ��¼��ʼ�����̸߳���
	}

}



///////////////////////////////////////////////// �̷߳���ʵ��
int Thread::generateID_ = 0;


// �̹߳���
Thread::Thread(ThreadFunc func)
	:func_(func),
	threadID_(generateID_++)
{}
Thread::~Thread()
{

}
// �����߳�
void Thread::start()
{
	//����һ���߳���ִ��һ���̺߳���
	std::thread t(func_, threadID_);	//c++11 t ���̺߳���func_
	t.detach();	//���÷����߳�
}

// ��ȡ�߳�ID
int Thread::getId() const
{
	return threadID_;
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

