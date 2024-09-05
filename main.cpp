#include<iostream>
//若使用模板类，需要包含头文件和源文件
#include"ThreadPool.h"
#include"ThreadPool.cpp"
#include"TaskQueue.h"
#include"TaskQueue.cpp"
#include<unistd.h>
using namespace std;


void Func(void* arg)
{
    int num = *(int*)arg;
    cout << "===== Thread ：" << pthread_self() << " is working, number： " << num <<"=====" << endl;
    sleep(1);
}
int main()
{
    //创建线程池 pool(min, max);
    ThreadPool<int> pool(2, 10);

    //循环向 线程池中添加新的任务
    for (int i = 0; i < 100; i++)
    {
        int* num = new int(i+100);

        pool.addTask(Task<int>(Func, num));
    }
    sleep(30);

    return 0;
}