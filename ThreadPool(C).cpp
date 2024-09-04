#include"ThreadPool(C).h"
#include<thread>
#include<mutex>
#include<string.h>
#include<unistd.h>
#include<condition_variable>
using namespace std;
const int NUMBER = 2;


//任务结构体
struct Task
{
	void(*function)(void* arg);
	void* arg;
};

//线程池结构体
struct ThreadPool
{
	//任务队列
	Task* taskQueue;
	int queueCapacity;
	int queueSize;
	int queueFront;
	int queueRear;

	//线程池：1 管理者线程; 2 工作的线程ID
	//存在共享内存，需要线程同步
	pthread_t managerID;
	pthread_t * threadsID;
	int minNum;		//最小线程
	int maxNum;		//最大线程
	int busyNum;	//忙碌线程
	int liveNum;	//存活线程
	int extiNum;	//销毁线程
	pthread_mutex_t mutexPool;	//锁整个线程池
	pthread_mutex_t mutexBusynum;	//给busyNum添加锁
	//条件变量阻塞生产者、消费者
	pthread_cond_t notFull;		//任务队列是不是满了
	pthread_cond_t notEmpty;	//任务队列是不是空了

	int shutdown;		//判断线程池是否工作 销毁为1 不销毁为0
};


ThreadPool* threadPoolCreate(int min, int max, int queueSize)
{
	//创建线程池的实例
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	
	do
	{
		if (pool == NULL)
		{
			cout << "malloc 失败" << endl;
			break;
		}

		pool->threadsID = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->threadsID == NULL)
		{
			cout << "malloc 失败" << endl;
			break;
		}
		memset(pool->threadsID, 0, sizeof(pthread_t) * max);

		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->liveNum = min;
		pool->extiNum = 0;

		//初始化互斥锁、条件变量
		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexBusynum, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0)

		{
			printf("mutex or condition init fail...\n");
			break;
		}

		//任务队列
		pool->taskQueue = (Task*)malloc(sizeof(Task) * queueSize);
		pool->queueCapacity = queueSize;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueRear = 0;

		pool->shutdown = 0;

		//创建线程
		pthread_create(&pool->managerID, NULL, manager, pool);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&pool->threadsID[i], NULL, worker, pool);
		}

		return pool;
	} while (0);

		//资源释放 3个
		if (pool && pool->threadsID)
		{
			free(pool->threadsID);
		}
		if (pool && pool->taskQueue)
		{
			free(pool->taskQueue);
		}
		if (pool)
		{
			free(pool);
		}

		return NULL;
}

//向线程池中添加任务
void addTask(ThreadPool* pool , void(*func)(void*), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	while (!pool->shutdown && pool->queueSize==pool->queueCapacity)
	{
		//说明线程池任务队列已满,阻塞生产者线程
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);

		if (!pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			return;
		}
		//添加任务
		pool->taskQueue[pool->queueRear].function = func;
		pool->taskQueue[pool->queueRear].arg = arg;
		pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
		pool->queueSize++;

		pthread_cond_signal(&pool->notEmpty);

	}
	pthread_mutex_unlock(&pool->mutexPool);
}


void* worker(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;

	while (1)
	{
		//读任务队列
		pthread_mutex_lock(&pool->mutexPool);
		//判断当前任务队列是否为空
		while (pool->queueSize == 0 && !pool->shutdown)
		{
			//阻塞工作线程
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			//判断是不是销毁
			if (pool->extiNum > 0)
			{
				pool->extiNum--;
				pool->liveNum--;
				pthread_mutex_unlock(&pool->mutexPool);
				ThreadExit(pool);
			}
		}

		//判断线程池是否关闭
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			ThredExit(pool);
		}
		// 从任务队列中取出一个任务
		Task task;
		task.function = pool->taskQueue[pool->queueFront].function;
		task.arg = pool->taskQueue[pool->queueFront].arg;
		//移动头节点 循环队列
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		//加锁、解锁
		//唤醒生产者
		pthread_cond_signal(&pool->notFull);
		pthread_mutex_unlock(&pool->mutexPool);

		cout << "thread" << pthread_self() << "is working..." << endl;
		pthread_mutex_lock(&pool->mutexBusynum);
		pool->busyNum++;
		pthread_mutex_unlock(&pool->mutexBusynum);
		task.function(task.arg);
		free(task.arg);
		task.arg = NULL;

		cout << "thread " << pthread_self() << " end working..." << endl;
		pthread_mutex_lock(&pool->mutexBusynum);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexBusynum);
	}
}

void* manager(void* arg)
{
	//类型转换
	ThreadPool* pool = (ThreadPool*)arg;
	//管理者线程 当线程池关闭的时候，管理者不起作用
	while (!pool->shutdown)
	{
		//检测需不需要进行调整工作线程
		sleep(3);
		//取出线程池中的任务数量和当前线程的数量
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->queueSize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);
		//获取线程池中的工作的线程
		pthread_mutex_lock(&pool->mutexBusynum);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusynum);

		// 创建线程
		// 任务的个数>存货的线程个数 && 存活的线程数 < 最大线程数
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int count = 0;
			for (int i = 0; i < pool->maxNum && count<NUMBER; i++)
			{
				//创建线程 判定 0 没被使用，若创建出的线程退出，线程ID修改成 0 
				if (pool->threadsID[i] == 0)
				{
					pthread_create(&pool->threadsID[i], NULL, worker, pool);
					count++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		//销毁线程
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->extiNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			//让工作线程自杀
			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return NULL;
}

void ThreadExit(ThreadPool* pool)
{
	//获取线程ID
	pthread_t tid = pthread_self();
	for (int i = 0; i < pool->maxNum; i++)
	{
		if (pool->threadsID[i] == tid)
		{
			pool->threadsID[i] = 0;
			cout << "threadExit() called, " << tid << "exiting..." << endl;
			break;
		}
	}
	pthread_exit(NULL);
}

//获取线程池中工作的线程个数
int threadPoolBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexBusynum);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusynum);
	return busyNum;
}

//获取线程池中活着的线程个数
int threadPoolLiveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexPool);
	int liveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return liveNum;
}

int destoryThread(ThreadPool* pool)
{
	//判断是否有效
	if (pool == NULL)
	{
		return -1;
	}

	//关闭线程池
	pool->shutdown = 1;
	//阻塞回收
	pthread_join(pool->managerID, NULL);
	//唤醒阻塞的消费者
	for (int i = 0; i < pool->liveNum; i++)
	{
		pthread_cond_signal(&pool->notEmpty);
	}
	//释放堆内存
	if (pool->taskQueue)
	{
		free(pool->taskQueue);
	}
	if (pool->threadsID)
	{
		free(pool->threadsID);
	}
	//销毁互斥锁
	pthread_mutex_destroy(&pool->mutexPool);
	pthread_mutex_destroy(&pool->mutexBusynum);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
	//销毁线程
	free(pool);
	pool = NULL;
	return 0;
}