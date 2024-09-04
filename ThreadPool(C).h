#pragma once
#include<iostream>
struct ThreadPool;
//��ʼ���̳߳�
ThreadPool* threadPoolCreate(int min, int max, int queueSize);



//�����̳߳صĺ���
int destoryThread(ThreadPool* pool);

//�������ĺ���
void addTask(ThreadPool* pool, void(*func)(void*), void* arg);


//��ȡ�̳߳��й������̸߳���
int threadPoolBusyNum(ThreadPool* pool);

//��ȡ�̳߳��л��ŵ��̸߳���
int threadPoolLiveNum(ThreadPool* pool);
//�������̣߳��������̣߳�������
void* worker(void* arg);
// ������߳�
void* manager(void* arg);

void ThredExit(ThreadPool* pool);

