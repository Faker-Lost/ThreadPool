#include<iostream>
#include"threadpool.h"
#include<chrono>
#include<thread>
#include<memory>

using ULong = unsigned long long;
class Mytask : public Task
{
public:
	Mytask(int begin, int end)
		:begin_(begin),
		end_(end)
	{}

	// 1. ��η�����������
	// C++ 17 Any ���ͣ����Խ���������������� tamplate
	// run���������̷߳�����߳���ȥִ����
	Any run()
	{
		std::cout << "Thread tid: " << std::this_thread::get_id() << " begin work..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		int sum = 0;
		for (int i = begin_; i <= end_; i++)
		{
			sum += i;
		}
		std::cout << "Thread tid: " << std::this_thread::get_id() << " end  work..." << std::endl;

		return sum;
	}
private:
	int begin_;
	int end_;
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
	{
		ThreadPool pool;
		pool.setMode(PoolMode::MODE_CACHED);
		pool.start(4);

		Result res1 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res2 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res3 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res4 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res5 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res6 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		Result res7 = pool.submitTask(std::make_shared<Mytask>(1, 100));
		int sum1 = res1.get().cast_<int>();
		int sum2 = res2.get().cast_<int>();
		int sum3 = res3.get().cast_<int>();
		int sum4 = res4.get().cast_<int>();
		int sum5 = res5.get().cast_<int>();
		int sum6 = res6.get().cast_<int>();
		int sum7 = res7.get().cast_<int>();
		std::cout << sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7 << std::endl;
	}
	getchar();
	return 0;


#if 0 
	{
		ThreadPool pool;
		// �����߳�ģʽ
		pool.setMode(PoolMode::MODE_CACHED);
		// �����̳߳�
		pool.start(4);

		// 2. ������Result�Ļ���
		Result res1 = pool.submitTask(std::make_shared<Mytask>(1, 100));

		Result res2 = pool.submitTask(std::make_shared<Mytask>(101, 200));

		Result res3 = pool.submitTask(std::make_shared<Mytask>(201, 300));

		pool.submitTask(std::make_shared<Mytask>(201, 300));


		pool.submitTask(std::make_shared<Mytask>(201, 300));
		pool.submitTask(std::make_shared<Mytask>(201, 300));

		// ����task��ִ���꣬task����û�ˣ�������task�����Result����Ҳû��
		int sum1 = res1.get().cast_<int>();

		int sum2 = res2.get().cast_<int>();

		int sum3 = res3.get().cast_<int>();

		// Master - Slave �߳�ģ��
		// Master�߳������ֽ����񣬸�Slave�̷߳�������
		// �ȴ�����Slave�߳�ִ�������񣬷��ؽ��
		// Master�̺߳ϲ����������������
		std::cout << "SUM : " << (sum1 + sum2 + sum3) << std::endl;
	}

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());

	//pool.submitTask(std::make_shared<Mytask>());


	getchar();
	return 0;
#endif
}