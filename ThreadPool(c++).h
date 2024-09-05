#pragma once
#include"TaskQueue.h"
using namespace std;
template<typename T>
class ThreadPool
{
public:

    ThreadPool<T>(int min, int max);
    ~ThreadPool<T>();

    // ������� ����������������
    void addTask(Task<T> task);
    // ��ȡæ�̵߳ĸ���
    int getBusyNumber();
    // ��ȡ���ŵ��̸߳���
    int getAliveNumber();

private:

    // �������̵߳�������
    static void* worker(void* arg);
    // �������̵߳�������
    static void* manager(void* arg);
    // �����߳��˳�
    void threadExit();

private:  
    // �������
    TaskQueue<T>* m_taskQ;

    pthread_mutex_t m_lock;
    pthread_cond_t m_notEmpty;          //��������

    pthread_t* m_threadIDs;
    pthread_t m_managerID;

    int m_minNum;
    int m_maxNum;
    int m_busyNum;
    int m_aliveNum;
    int m_exitNum;
    bool m_shutdown = false;
};

