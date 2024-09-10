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
#include<unordered_map>
#include<iostream>

// Any�ࣺ���Խ����������ݵ�����
class Any
{
public:
	Any() = default;
	~Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;

	// ���캯������Any��������������
	template<typename T>
	Any(T data) :base_ptr(std::make_unique<Deriver<T>>(data))
	{}

	// ��������ܰ�Any��������洢��data������ȡ����
	template<typename T>
	T cast_()
	{
		// ����ָ��ת�� ������ָ��
		Deriver<T>* ptr = dynamic_cast<Deriver<T>*>(base_ptr.get());
		if (ptr == nullptr)
		{
			std::cout << "ptr is nullptr!" << std::endl;
		}
		return ptr->data_;
	}

private:
	// ��������
	class Base
	{
	public:
		virtual ~Base() = default;
	};

	template<typename T>
	class Deriver :public Base
	{
	public:
		Deriver(T data) :data_(data)
		{}
		T data_;
	};

private:
	// �������ָ��
	std::unique_ptr<Base> base_ptr;
};


// ʵ���ź�����: 1.������ + �ź���
class Semaphore
{
public:
	Semaphore(int limit = 0)
		:resLimit_(limit)
	{}
	~Semaphore() = default;

	// ��ȡһ���ź�����Դ
	void wait()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		cond_.wait(lock, [&]()->bool { return resLimit_ > 0; });
		resLimit_--;
	}

	// ����һ���ź�����Դ
	void post()
	{
		std::unique_lock<std::mutex> lock(mtx_);
		resLimit_++;
		cond_.notify_all();
	}
private:
	int resLimit_;
	//������
	std::mutex mtx_;
	// ��������
	std::condition_variable cond_;
};

// ʵ�ֽ����ύ���̳߳ص�Task����ִ����ɺ󷵻�ֵResult
class Task;		//taskǰ������

class Result
{
public:
	Result(std::shared_ptr<Task> task, bool isValid = true);
	~Result() = default;

	// setVal ��ȡ����ִ����ķ���ֵ
	void setVal(Any any);
	// get()����ȡtask����ֵ
	Any get();

private:
	Any any_;	//�洢�������͵ķ���ֵ
	Semaphore sem_;		//�ź���
	std::shared_ptr<Task> task_;	//ָ���Ӧ��ȡ����ֵ���������
	std::atomic_bool isValid_;		//����ֵ�Ƿ���Ч	
};


// ����������
//�û������Զ��������������ͣ���Task�̳У���дrun������ʵ���Զ�����������
class Task
{
public:
	Task();
	~Task() = default;
	void exec();
	void setResult(Result* res);

	// �����������͵ķ���ֵ
	virtual Any run() = 0;

private:
	Result* result_;
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
	using ThreadFunc = std::function<void(int)>;
	//�̹߳���
	Thread(ThreadFunc func);
	~Thread();
	//�����߳�
	void start();

	// ��ȡ�߳�id
	int getId() const;
private:
	ThreadFunc func_;
	static int generateID_;
	int threadID_;		// �����߳�ID
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

	// ����cachedģʽ���̵߳���ֵ
	void setThreadSizeMaxHold(size_t threadSizeMaxHold_);

	// �����̳߳� ��ǰϵͳCPU��������
	void start(int  initThreadSize = std::thread::hardware_concurrency());

	// ��������������޵���ֵ
	void setTaskQueMaxThreadHold(int threadhold);

	//���̳߳��ύ����
	Result submitTask(std::shared_ptr<Task> sp);


	ThreadPool(const ThreadPool&) = delete;
	ThreadPool operator=(const ThreadPool&) = delete;
private:
	// �����̺߳���
	void threadFunc(int threadid);

	// ��鵱ǰ�߳�����״̬
	bool checkRunning() const;

private:
	std::unordered_map<int, std::unique_ptr<Thread> > threads_;	// �߳��б�

	size_t initThreadSize_;							// ��ʼ�߳�����
	size_t threadSizeMaxHold_;						// �߳���������
	std::atomic_int curThreadSize_;					// ��¼��ǰ�߳������̵߳�����
	std::atomic_int idleThreadSize_;				// ��¼�����߳�����

	std::queue<std::shared_ptr<Task>> taskQueue_;	//�������
	std::atomic_int taskSize_; 						//ԭ������ ���������
	size_t taskQueMaxThreadHold_;					//��������������޵���ֵ
	
	/*
	* �����������������
	* 1. ���գ��������߳̿�������
	* 2. �������������߳̿�������
	*/
	std::mutex	taskQueMutex_;						//��֤������е��̰߳�ȫ
	std::condition_variable notFull_;				//������в���
	std::condition_variable notEmpty_;				//������в���
	std::condition_variable exitCond_;				// �ȴ��߳���Դȫ������

	PoolMode poolMode_;								//��ǰ�̳߳صĹ���ģʽ
	std::atomic_bool isPoolRunning_;				// ��ʾ��ǰ�̳߳ص�����״̬


};



#endif
