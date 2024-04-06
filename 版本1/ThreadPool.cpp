#include "ThreadPool.h"
#include <mutex>
#include <memory>
#include <thread>
#include <functional>

const int THREADMAXSIZE = 40;//线程最大数量
const int TASKMAXSIZE = 100;//任务最大数量
const int THREAD_MAX_IDLE_TIME = 5;//线程最大空闲时间
static int threadid = 0;

//线程池
ThreadPool::ThreadPool()
	:initThreadSize(0),
	taskSize(0),
	threadMaxSize(THREADMAXSIZE),
	taskMaxSize(TASKMAXSIZE),
	threadPoolMode(FIXED)
{}
ThreadPool::~ThreadPool()
{
	isRunning = false;
	
	std::unique_lock<std::mutex> lck(taskQueMutex);
	isEmpty.notify_all();
	cvClosed.wait(lck, [this]() {return threads.size() == 0; });
}

void ThreadPool::setMode(int mode)
{
	threadPoolMode = mode;
}

//启动线程池
void ThreadPool::start(int size)
{
	isRunning = true;
	initThreadSize = size;
	for (int i = 0; i < initThreadSize; i++)
	{
		auto t = std::make_unique<Thread>([this](int a) {this->threadFunc(a); });
		int tid = t->getId();
		threads.emplace(tid, std::move(t));
		
	}
	for (int i = 0; i < initThreadSize; i++)
	{
		threads[i]->start();
		std::cout << "thread created" << std::endl;
		curThreadSize++;
		freeThreadSize++;
	}
}

//线程函数
void ThreadPool::threadFunc(int id)
{
	auto lastTime = std::chrono::high_resolution_clock().now();
	for(;;)
	{
		std::shared_ptr<Task> task1;
		{
			std::unique_lock<std::mutex> lck(taskQueMutex);
			std::cout << std::this_thread::get_id() << "线程尝试获取任务..." << std::endl;
			while (taskQue.size() == 0)
			{
				if (isRunning == false)
				{
					threads.erase(id);
					std::cout << std::this_thread::get_id() << " deleted !" << std::endl;
					cvClosed.notify_all();
					return;
				}
				if (threadPoolMode == CACHED)
				{
					if (std::cv_status::timeout ==
						isEmpty.wait_for(lck, std::chrono::seconds(1)))
					{
						auto now = std::chrono::high_resolution_clock().now();
						auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
						if (dur.count() >= THREAD_MAX_IDLE_TIME
							&& curThreadSize > initThreadSize)
						{
							threads.erase(id);
							curThreadSize--;
							freeThreadSize--;
							std::cout << std::this_thread::get_id() << "deleted! " << std::endl;
							return;
						}
					}
				}
				else
				{
					isEmpty.wait(lck);//task为空，进入等待状态
				}
				
			}
			task1 = taskQue.front();
			std::cout << std::this_thread::get_id() << "线程获取任务成功！" << std::endl;
			freeThreadSize--;
			taskQue.pop();
			taskSize--;
			isFull.notify_all();//唤醒生产者线程
		}
		task1->exec();//执行任务
		freeThreadSize++;
		lastTime = std::chrono::high_resolution_clock().now();//更新时间
	}

}

//提交任务
Result ThreadPool::submitTask(std::shared_ptr<Task> task1)
{
	
	std::unique_lock<std::mutex> lck(taskQueMutex);
	if (!isFull.wait_for(lck, std::chrono::seconds(1),
		[this]() {return taskQue.size() < taskMaxSize; }))
	{
		std::cout << "taskQue is full , submit fail!" << std::endl;

	}
	taskQue.push(task1);//提交任务
	taskSize++;
	isEmpty.notify_all();//唤醒等待状态的线程
	
	std::cout << "提交任务成功" << std::endl;

	while (threadPoolMode == CACHED && freeThreadSize < taskSize &&
		curThreadSize < threadMaxSize)
	{
		auto t = std::make_unique<Thread>([this](int a) {threadFunc(a); });
		int tid = t->getId();
		threads.emplace(tid, std::move(t));
		threads[tid]->start();
		freeThreadSize++;
		curThreadSize++;
		std::cout << "new thread " << "created! " << std::endl;
	}
	return Result(task1);
}

//////////////////////线程
Thread::Thread(std::function<void(int)> func1)
	:func(func1),
	id(threadid++)
{}
Thread::~Thread()
{}
void Thread::start()
{
	std::thread t(func, this->id);
	t.detach();
}
int Thread::getId()
{
	return id;
}

//////////////////任务
Task::Task()
{
	result_ = nullptr;
}

Task::~Task()
{}

void Task::exec()
{
	result_->setVal(run());
}

/////////////////Result 
Result::Result(std::shared_ptr<Task> task1)
	:task(task1)
{
	if(task != nullptr)
		task->result_ = this;
}
Result::~Result()
{}
void Result::setVal(Any any1)
{
	any = std::move(any1);
	sem.P();
}
Any Result::getVal() 
{
	sem.V();
	return std::move(any);
}