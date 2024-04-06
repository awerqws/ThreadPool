// 线程池项目终版.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <thread>
#include <future>
#include "ThreadPool.h"
using namespace std;
int func1(int a, int b)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return a + b;
}
int main()
{
    auto f = [](int a, int b)->void {  a + b; };
    f(1, 2);
    /*{
        ThreadPool pool;
        pool.setMode(CACHED);
        pool.start(4);

        future<int> r1 = pool.submitTask(func1, 10, 20);
        future<int> r2 = pool.submitTask(func1, 10, 20);
        future<int> r3 = pool.submitTask([](int a, int b)->int {
            int sum = 0;
            for (int i = a; i <= b; i++)
                sum += i;
            return sum;
            }, 10, 20);
        future<int> r4 = pool.submitTask(func1, 10, 20);
        future<int> r5 = pool.submitTask(func1, 10, 20);
        future<int> r6 = pool.submitTask(func1, 10, 20);
        future<int> r7 = pool.submitTask(func1, 10, 20);
        future<int> r8 = pool.submitTask(func1, 10, 20);

        cout << r1.get() << endl;
        cout << r2.get() << endl;
        cout << r3.get() << endl;
        cout << r4.get() << endl;
        cout << r5.get() << endl;
        cout << r6.get() << endl;
    }*/
    getchar();
}


