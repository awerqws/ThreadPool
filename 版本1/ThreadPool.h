#pragma once
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>

//线程池两种模式
enum {
	FIXED,//固定线程数量
	CACHED,//线程数量可以增加
};

class Any
{
public:
	template<class T>
	Any(T data)
	{
		base = std::make_unique<Derive<T>>(data);
	}
	Any() = default;
	Any(const Any&) = delete;
	Any& operator=(const Any&) = delete;
	Any(Any&&) = default;
	Any& operator=(Any&&) = default;
	~Any(){}
	template<class T>
	T cast_()
	{
		Derive<T>* p = dynamic_cast<Derive<T>*>(base.get());
		if (p == nullptr)
		{
			throw "type is unmatch";
		}
		return p->data;
	}

private:
	class Base
	{
	public:
		Base() = default;
		virtual ~Base() = default;
	private:
	};
	template<class T>
	class Derive :public Base
	{
	public:
		Derive(T data1):data(data1){}
		~Derive(){}
		T data;
	};
private:
	std::unique_ptr<Base> base;
};

//信号量类
class Semphore
{
public:
	Semphore(int number = 0)
		:number(number)
	{}
	Semphore(Semphore&) = delete;
	~Semphore() = default;
	void P()
	{
		std::unique_lock<std::mutex> lck(mtx);
		number++;
		cv.notify_all();
	}
	void V()
	{
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck, [this]()->bool {return number > 0; });
		number--;
		//cv.notify_all();
	}
private:
	int number;
	std::mutex mtx;
	std::condition_variable cv;
};

//线程类
class Thread
{
public:
	Thread(std::function<void(int)> func1);
	Thread(const Thread&) = delete;
	~Thread();
	void start();//启动线程
	int getId();
private:
	std::function<void(int)> func;
	int id;
};
class Result;
//任务类
class Task
{
public:
	Task();
	virtual ~Task();
	Any virtual run() = 0;//用户定义task类继承Task，重写该方法
	void exec();
	Result* result_;
};

//Result类，提交任务返回的结果类
class Result
{
public:
	Result(std::shared_ptr<Task> task1);
	~Result();
	Result(Result&) = delete;
	void setVal(Any any1);
	Any getVal();
private:
	std::shared_ptr<Task> task;
	Any any;
	Semphore sem;
};

//线程池类
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	ThreadPool(ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	void start(int size = std::hardware_constructive_interference_size); //启动线程池
	Result submitTask(std::shared_ptr<Task> task1);//用户提交任务，任务继承于Task类，传入基类的指针
	void setMode(int mode);//设置线程池模式
private:
	int threadPoolMode;//线程池模式
	bool isRunning;//线程池运行状态
	std::condition_variable cvClosed;
	std::mutex closeMutex;

	std::unordered_map<int, std::unique_ptr<Thread>> threads;//线程队列
	int initThreadSize;//初始线程数量
	int threadMaxSize;//线程最大数量
	std::atomic_int curThreadSize;//当前线程数量
	std::atomic_int freeThreadSize; // 当前空闲线程数量

	void threadFunc(int threadid);//线程函数

	std::queue<std::shared_ptr<Task>> taskQue;//任务队列
	std::atomic_int taskSize;//任务数量，设为原子类型
	int taskMaxSize;//任务最大数量

	std::mutex taskQueMutex;//保证任务队列的线程安全
	std::condition_variable isEmpty;
	std::condition_variable isFull; //条件变量，用于线程通信
};

#endif 
