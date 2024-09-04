#pragma once
#include<iostream>
struct ThreadPool;
//初始化线程池
ThreadPool* threadPoolCreate(int min, int max, int queueSize);



//销毁线程池的函数
int destoryThread(ThreadPool* pool);

//添加任务的函数
void addTask(ThreadPool* pool, void(*func)(void*), void* arg);


//获取线程池中工作的线程个数
int threadPoolBusyNum(ThreadPool* pool);

//获取线程池中活着的线程个数
int threadPoolLiveNum(ThreadPool* pool);
//工作的线程（消费者线程）任务函数
void* worker(void* arg);
// 管理的线程
void* manager(void* arg);

void ThredExit(ThreadPool* pool);

