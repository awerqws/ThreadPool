#include "ThreadPool.h"
#include <iostream>
#include <thread>
class myTask :public Task
{
public:
	myTask(long long a, long long b)
		:a(a), b(b)
	{};
	~myTask() {};
	Any run()
	{
		
		long long sum = 0;
		for (long long i = a; i <= b; i++)
		{
			sum += i;
		}
		Any any(sum);
		return any;
	}
private:
	long long a;
	long long b;
};

int main()
{
	{
		
		ThreadPool pool;
		pool.setMode(CACHED);
		pool.start(4);
		
		

		Result r1 = pool.submitTask(std::make_shared<myTask>(myTask(1, 1000000000)));
		Result r2 = pool.submitTask(std::make_shared<myTask>(myTask(1000000001, 2000000000)));
		Result r3 = pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));
		pool.submitTask(std::make_shared<myTask>(myTask(2000000001, 3000000000)));

		long long n1 = r1.getVal().cast_<long long>();
		long long n2 = r2.getVal().cast_<long long>();
		long long n3 = r3.getVal().cast_<long long>();
		

		std::cout << n1 + n2 + n3 << std::endl;

		//pool.submitTask(p1);
		//pool.submitTask(p2);
		//pool.submitTask(p3);
		//pool.submitTask(p4);
		//pool.submitTask(p5);
		//pool.submitTask(p6);

		
	}
	
}