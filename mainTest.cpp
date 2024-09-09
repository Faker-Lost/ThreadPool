#include<iostream>
#include"threadpool.h"
#include<chrono>
#include<thread>
#include<memory>
class Mytask : public Task
{
public:
	Mytask(int begin, int end)
		:begin_(begin),
		end_(end)
	{}

	// 1. 如何返回任意类型
	// C++ 17 Any 类型，可以接受任意的其他类型 tamplate
	// run方法就在线程分配的线程中去执行了
	Any run()
	{
		std::cout << "tid: " << std::this_thread::get_id() << " begin..." << std::endl;

		int sum = 0;
		for (int i = begin_; i <= end_; i++)
		{
			sum += i;
		}
		std::cout << "tid: " << std::this_thread::get_id() << " end..." << std::endl;

		return sum;
	}
private:
	int begin_;
	int end_;
};

/*有些场景，是希望能够获取线程执行任务得到返回值
example：
	1 + ... + 10000
	10000 + ... +
	10001 + ... +
	最后得到一个合并结果
*/


int main()
{
	ThreadPool pool;
	// 设置线程模式
	pool.setMode(PoolMode::MODE_CACHED);
	// 启动线程池
	pool.start(4);

	// 2. 如何设计Result的机制
	Result res1 = pool.submitTask(std::make_shared<Mytask>(1, 100));

	Result res2 = pool.submitTask(std::make_shared<Mytask>(101, 200));

	Result res3 = pool.submitTask(std::make_shared<Mytask>(201, 300));


	// 随着task被执行完，task对象没了，依赖于task对象的Result对象也没了
	int sum1 = res1.get().cast_<int>();

	int sum2 = res2.get().cast_<int>();

	int sum3 = res3.get().cast_<int>();

	// Master - Slave 线程模型
	// Master线程用来分解任务，给Slave线程分配任务
	// 等待各个Slave线程执行完任务，返回结果
	// Master线程合并各个任务结果，输出
	std::cout << "SUM : " << (sum1 + sum2 + sum3) << std::endl;

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());



	return 0;
}