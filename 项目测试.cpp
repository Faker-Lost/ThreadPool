#include<iostream>
#include"threadpool.h"
#include<chrono>
#include<thread>
#include<memory>
class Mytask:public Task
{
public:
	// 1. ��η�����������
	// C++ 17 Any ���ͣ����Խ���������������� tamplate
	void run()
	{
		std::cout << "tid: " << std::this_thread::get_id() <<" begin..." << std::endl;

		std::this_thread::sleep_for(std::chrono::seconds(2));

		std::cout << "tid: " << std::this_thread::get_id() << " end..." << std::endl;
	}
};

/*��Щ��������ϣ���ܹ���ȡ�߳�ִ������õ�����ֵ
example��
	1 + ... + 10000
	10000 + ... + 
	10001 + ... + 
	���õ�һ���ϲ����
*/


int main()
{
	ThreadPool pool;
	pool.start(4);

	// 2. ������Result�Ļ���
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