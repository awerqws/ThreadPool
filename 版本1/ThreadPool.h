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

//�̳߳�����ģʽ
enum {
	FIXED,//�̶��߳�����
	CACHED,//�߳�������������
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

//�ź�����
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

//�߳���
class Thread
{
public:
	Thread(std::function<void(int)> func1);
	Thread(const Thread&) = delete;
	~Thread();
	void start();//�����߳�
	int getId();
private:
	std::function<void(int)> func;
	int id;
};
class Result;
//������
class Task
{
public:
	Task();
	virtual ~Task();
	Any virtual run() = 0;//�û�����task��̳�Task����д�÷���
	void exec();
	Result* result_;
};

//Result�࣬�ύ���񷵻صĽ����
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

//�̳߳���
class ThreadPool
{
public:
	ThreadPool();
	~ThreadPool();
	ThreadPool(ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
	void start(int size = std::hardware_constructive_interference_size); //�����̳߳�
	Result submitTask(std::shared_ptr<Task> task1);//�û��ύ��������̳���Task�࣬��������ָ��
	void setMode(int mode);//�����̳߳�ģʽ
private:
	int threadPoolMode;//�̳߳�ģʽ
	bool isRunning;//�̳߳�����״̬
	std::condition_variable cvClosed;
	std::mutex closeMutex;

	std::unordered_map<int, std::unique_ptr<Thread>> threads;//�̶߳���
	int initThreadSize;//��ʼ�߳�����
	int threadMaxSize;//�߳��������
	std::atomic_int curThreadSize;//��ǰ�߳�����
	std::atomic_int freeThreadSize; // ��ǰ�����߳�����

	void threadFunc(int threadid);//�̺߳���

	std::queue<std::shared_ptr<Task>> taskQue;//�������
	std::atomic_int taskSize;//������������Ϊԭ������
	int taskMaxSize;//�����������

	std::mutex taskQueMutex;//��֤������е��̰߳�ȫ
	std::condition_variable isEmpty;
	std::condition_variable isFull; //���������������߳�ͨ��
};

#endif 
