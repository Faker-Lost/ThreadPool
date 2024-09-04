#include"ThreadPool(C).h"
#include<thread>
#include<mutex>
#include<string.h>
#include<unistd.h>
#include<condition_variable>
using namespace std;
const int NUMBER = 2;


//����ṹ��
struct Task
{
	void(*function)(void* arg);
	void* arg;
};

//�̳߳ؽṹ��
struct ThreadPool
{
	//�������
	Task* taskQueue;
	int queueCapacity;
	int queueSize;
	int queueFront;
	int queueRear;

	//�̳߳أ�1 �������߳�; 2 �������߳�ID
	//���ڹ����ڴ棬��Ҫ�߳�ͬ��
	pthread_t managerID;
	pthread_t * threadsID;
	int minNum;		//��С�߳�
	int maxNum;		//����߳�
	int busyNum;	//æµ�߳�
	int liveNum;	//����߳�
	int extiNum;	//�����߳�
	pthread_mutex_t mutexPool;	//�������̳߳�
	pthread_mutex_t mutexBusynum;	//��busyNum�����
	//�����������������ߡ�������
	pthread_cond_t notFull;		//��������ǲ�������
	pthread_cond_t notEmpty;	//��������ǲ��ǿ���

	int shutdown;		//�ж��̳߳��Ƿ��� ����Ϊ1 ������Ϊ0
};


ThreadPool* threadPoolCreate(int min, int max, int queueSize)
{
	//�����̳߳ص�ʵ��
	ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
	
	do
	{
		if (pool == NULL)
		{
			cout << "malloc ʧ��" << endl;
			break;
		}

		pool->threadsID = (pthread_t*)malloc(sizeof(pthread_t) * max);
		if (pool->threadsID == NULL)
		{
			cout << "malloc ʧ��" << endl;
			break;
		}
		memset(pool->threadsID, 0, sizeof(pthread_t) * max);

		pool->minNum = min;
		pool->maxNum = max;
		pool->busyNum = 0;
		pool->liveNum = min;
		pool->extiNum = 0;

		//��ʼ������������������
		if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
			pthread_mutex_init(&pool->mutexBusynum, NULL) != 0 ||
			pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
			pthread_cond_init(&pool->notFull, NULL) != 0)

		{
			printf("mutex or condition init fail...\n");
			break;
		}

		//�������
		pool->taskQueue = (Task*)malloc(sizeof(Task) * queueSize);
		pool->queueCapacity = queueSize;
		pool->queueSize = 0;
		pool->queueFront = 0;
		pool->queueRear = 0;

		pool->shutdown = 0;

		//�����߳�
		pthread_create(&pool->managerID, NULL, manager, pool);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&pool->threadsID[i], NULL, worker, pool);
		}

		return pool;
	} while (0);

		//��Դ�ͷ� 3��
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

//���̳߳����������
void addTask(ThreadPool* pool , void(*func)(void*), void* arg)
{
	pthread_mutex_lock(&pool->mutexPool);
	while (!pool->shutdown && pool->queueSize==pool->queueCapacity)
	{
		//˵���̳߳������������,�����������߳�
		pthread_cond_wait(&pool->notFull, &pool->mutexPool);

		if (!pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			return;
		}
		//�������
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
		//���������
		pthread_mutex_lock(&pool->mutexPool);
		//�жϵ�ǰ��������Ƿ�Ϊ��
		while (pool->queueSize == 0 && !pool->shutdown)
		{
			//���������߳�
			pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);
			//�ж��ǲ�������
			if (pool->extiNum > 0)
			{
				pool->extiNum--;
				pool->liveNum--;
				pthread_mutex_unlock(&pool->mutexPool);
				ThreadExit(pool);
			}
		}

		//�ж��̳߳��Ƿ�ر�
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexPool);
			ThredExit(pool);
		}
		// �����������ȡ��һ������
		Task task;
		task.function = pool->taskQueue[pool->queueFront].function;
		task.arg = pool->taskQueue[pool->queueFront].arg;
		//�ƶ�ͷ�ڵ� ѭ������
		pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
		pool->queueSize--;
		//����������
		//����������
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
	//����ת��
	ThreadPool* pool = (ThreadPool*)arg;
	//�������߳� ���̳߳عرյ�ʱ�򣬹����߲�������
	while (!pool->shutdown)
	{
		//����費��Ҫ���е��������߳�
		sleep(3);
		//ȡ���̳߳��е����������͵�ǰ�̵߳�����
		pthread_mutex_lock(&pool->mutexPool);
		int queueSize = pool->queueSize;
		int liveNum = pool->liveNum;
		pthread_mutex_unlock(&pool->mutexPool);
		//��ȡ�̳߳��еĹ������߳�
		pthread_mutex_lock(&pool->mutexBusynum);
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexBusynum);

		// �����߳�
		// ����ĸ���>������̸߳��� && �����߳��� < ����߳���
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			int count = 0;
			for (int i = 0; i < pool->maxNum && count<NUMBER; i++)
			{
				//�����߳� �ж� 0 û��ʹ�ã������������߳��˳����߳�ID�޸ĳ� 0 
				if (pool->threadsID[i] == 0)
				{
					pthread_create(&pool->threadsID[i], NULL, worker, pool);
					count++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexPool);
		}

		//�����߳�
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexPool);
			pool->extiNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexPool);
			//�ù����߳���ɱ
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
	//��ȡ�߳�ID
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

//��ȡ�̳߳��й������̸߳���
int threadPoolBusyNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexBusynum);
	int busyNum = pool->busyNum;
	pthread_mutex_unlock(&pool->mutexBusynum);
	return busyNum;
}

//��ȡ�̳߳��л��ŵ��̸߳���
int threadPoolLiveNum(ThreadPool* pool)
{
	pthread_mutex_lock(&pool->mutexPool);
	int liveNum = pool->liveNum;
	pthread_mutex_unlock(&pool->mutexPool);
	return liveNum;
}

int destoryThread(ThreadPool* pool)
{
	//�ж��Ƿ���Ч
	if (pool == NULL)
	{
		return -1;
	}

	//�ر��̳߳�
	pool->shutdown = 1;
	//��������
	pthread_join(pool->managerID, NULL);
	//����������������
	for (int i = 0; i < pool->liveNum; i++)
	{
		pthread_cond_signal(&pool->notEmpty);
	}
	//�ͷŶ��ڴ�
	if (pool->taskQueue)
	{
		free(pool->taskQueue);
	}
	if (pool->threadsID)
	{
		free(pool->threadsID);
	}
	//���ٻ�����
	pthread_mutex_destroy(&pool->mutexPool);
	pthread_mutex_destroy(&pool->mutexBusynum);
	pthread_cond_destroy(&pool->notEmpty);
	pthread_cond_destroy(&pool->notFull);
	//�����߳�
	free(pool);
	pool = NULL;
	return 0;
}