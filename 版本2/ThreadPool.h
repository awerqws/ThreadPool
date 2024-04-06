#ifndef THREADPOOL_H
#define THREADPOOL_H
#include <unordered_map>
#include <memory>
#include <future>
#include <semaphore>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <iostream>
#include <thread>
#include <atomic>
#include <functional>

const int TASK_MAX_SIZE = 10;
const int THREAD_MAX_SIZE = 1024;
const int THREAD_MAX_IDLE_TIME = 60;//线程最大空闲时间

//线程池模式
enum {
	FIXED,
	CACHED
};

//线程类
class Thread
{
public:
	Thread(std::function<void(int)> func1)
		:threadId(id++),
		func(func1)
	{}
	//Thread(Thread&) = delete;
	//Thread& operator=(const Thread&) = delete;
	~Thread() = default;
	//Thread(Thread&&) = default;
	//Thread& operator=(Thread&&) = default;
	int getId()
	{
		return threadId;
	}
	void start()
	{
		std::thread t1(func, threadId);
		
		t1.detach();
	}
private:
	int threadId;
	static int id;
	std::function<void(int)> func;
};
int Thread::id = 0;
class ThreadPool
{
public:
	ThreadPool()
		:ThreadPoolMode(FIXED),
		isRunning(false),
		initThreadSize(0),
		threadMaxSize(THREAD_MAX_SIZE),
		taskMaxSize(TASK_MAX_SIZE),
		curThreadSize(0),
		freeThreadSize(0),
		curTaskSize(0)
	{}
	ThreadPool(ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	~ThreadPool()
	{
		isRunning = false;
		isEmpty.notify_all();
		std::unique_lock<std::mutex> lock(taskQueMutex);
		exit.wait(lock, [this]()->bool {return threads.size() == 0; });
	}
	void start(int size = 4)//启动
	{
		isRunning = true;
		initThreadSize = size;
		curThreadSize = size;

		for (int i = 0; i < initThreadSize; i++)
		{
			auto t = std::make_unique<Thread>([this](int a) {threadFunc(a); });
			int id = t->getId();
			threads.emplace(id, std::move(t));
		}
		for (int i = 0; i < initThreadSize; i++)
		{
			threads[i]->start();
			curThreadSize++;
			freeThreadSize++;
		}
	}
	void setMode(int mode)//设置模式
	{
		if (isRunning = true)
		{
			std::cout << "ThreadPool is running, con't set mode again." << std::endl;
			return;
		}
		ThreadPoolMode = mode;
	}
	void setTaskMax(int maxSize)//设置任务最大上限
	{
		if (isRunning = true)
		{
			std::cout << "ThreadPool is running, con't set maxSize." << std::endl;
			return;
		}
		taskMaxSize = maxSize;
	}
	template<class Func, class...Args>
	auto submitTask(Func &&func, Args&&...args) -> std::future<decltype(func(args...))>
	{
		typedef  decltype(func(args...)) Rtype;
		auto task = std::make_shared<std::packaged_task<Rtype()>>(
			[=]() {return func(args...); }
		);
		std::future<Rtype> result = task->get_future();

		std::unique_lock<std::mutex> lock(taskQueMutex);

		if (curTaskSize == taskMaxSize)
		{
			isFull.wait(lock);
		}
		//Task类型：std::function<void()>
		taskQueue.emplace(([task]() {(*task)(); }));//使用lamada对(*task)()进行了一层封装，使任务返回值为void
		curTaskSize++;
		isEmpty.notify_all();
		std::cout << "Submit task successfully! " << std::endl;
		if (ThreadPoolMode == CACHED && curTaskSize > freeThreadSize)
		{
			auto t = std::make_unique<Thread>([this](int a) {threadFunc(a); });
			int id = t->getId();
			threads.emplace(id, std::move(t));
			std::cout << "New thread created! " << std::endl;
			threads[id]->start();
			curThreadSize++;
			freeThreadSize++;
		}
		return result;
	}

private:
	int ThreadPoolMode;//线程池模式
	bool isRunning;//当前线程池状态
	std::condition_variable exit;//线程池回收资源

	std::unordered_map<int, std::unique_ptr<Thread>> threads;//线程队列
	int initThreadSize;//初始线程数量
	int threadMaxSize;//线程最大数量
	std::atomic_int curThreadSize;//当前线程数量
	std::atomic_int freeThreadSize;//当前空闲线程数量

	using Task = std::function<void()>;
	std::queue<Task> taskQueue;//任务队列
	std::atomic_int curTaskSize;//当前任务数量
	int taskMaxSize;//任务最大数量
	std::mutex taskQueMutex;//任务队列互斥锁
	std::condition_variable isEmpty;//是否为空
	std::condition_variable isFull;//是否满

	void threadFunc(int id)//线程函数
	{
		auto lastTime = std::chrono::high_resolution_clock().now();
		
		for (;;)
		{
			Task task1;
			{
				std::unique_lock<std::mutex> lock(taskQueMutex);
				std::cout << std::this_thread::get_id() << "Try get task..." << std::endl;
				while (isRunning && taskQueue.size() == 0)
				{
					if (ThreadPoolMode == CACHED)
					{
						if (std::cv_status::timeout ==
							isEmpty.wait_for(lock, std::chrono::seconds(1)))
						{
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
							if (dur.count() >= THREAD_MAX_IDLE_TIME
								&& curThreadSize > initThreadSize)//超过线程最大等待时间而且线程数量大于初始线程数量
							{
								threads.erase(id);
								freeThreadSize--;
								curThreadSize--;
								std::cout << std::this_thread::get_id() << "is deleted! " << std::endl;
								return;
							}
						}
					}
					else {
						isEmpty.wait(lock);
					}
					
				}
				if (isRunning == false)
				{
					break;
				}
				task1 = taskQueue.front();
				taskQueue.pop();
				curTaskSize--;
				freeThreadSize--;
				std::cout << std::this_thread::get_id() << "Get task successfully！" << std::endl;
				isFull.notify_all();
				if (taskQueue.size() > 0)
				{
					isEmpty.notify_all();
				}
			}
			task1();
			freeThreadSize++;
			lastTime = std::chrono::high_resolution_clock().now();
		}
		threads.erase(id);
		curThreadSize--;
		freeThreadSize--;
		std::cout << std::this_thread::get_id() << " is deleted! " << std::endl;
		exit.notify_all();
	}

};

#endif // !THREADPOOL_H
