#include<iostream>
#include"threadpool.h"
#include<chrono>
#include<thread>
#include<memory>
class Mytask:public Task
{
public:
	// 1. 如何返回任意类型
	// C++ 17 Any 类型，可以接受任意的其他类型 tamplate
	void run()
	{
		std::cout << "tid: " << std::this_thread::get_id() <<" begin..." << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		std::cout << "tid: " << std::this_thread::get_id() << " end..." << std::endl;
	}
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
	pool.start(4);

	// 2. 如何设计Result的机制
	Result res = pool.submitTask(std::make_shared<Mytask>());
	res.get();

	pool.submitTask(std::make_shared<Mytask>());

	pool.submitTask(std::make_shared<Mytask>());

	pool.submitTask(std::make_shared<Mytask>());

	pool.submitTask(std::make_shared<Mytask>());

	pool.submitTask(std::make_shared<Mytask>());


	getchar();
	
	return 0;
}