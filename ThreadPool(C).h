#pragma once
#include<iostream>
struct ThreadPool;
//��ʼ���̳߳�
ThreadPool* threadPoolCreate(int min, int max, int queueSize);



//�����̳߳صĺ���


//�������ĺ���
void addTask(ThreadPool* pool, void(*func)(void*), void* arg);


//��ȡ�̳߳��й������̸߳���

//��ȡ�̳߳��л��ŵ��̸߳���

//�������̣߳��������̣߳�������
void* worker(void* arg);
// ������߳�
void* manager(void* arg);

void ThredExit(ThreadPool* pool);

