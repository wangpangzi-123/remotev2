#pragma once
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>

class ThreadFuncBase {};
using FUNCTYPE = int (ThreadFuncBase::*)();


class ThreadWorker {
public:
	ThreadWorker() : thiz(NULL), func(NULL) {}

	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f)
		: thiz(obj), func(f){}

	ThreadWorker(const ThreadWorker& worker){
		thiz = worker.thiz;
		func = worker.func;
	}

	ThreadWorker& operator= (const ThreadWorker& worker)
	{
		if (this != &worker)
		{
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}

	int operator()() {
		if (IsValid())
		{
			return (thiz->*func)();
		}
		return -1;
	}

	bool IsValid() const
	{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class LThread
{
public:
	LThread()
		: m_hThread(NULL), m_bState(false)
	{}

	~LThread()
	{
		Stop();
	}

	bool Start()
	{
		m_bState = true;
		m_hThread = (HANDLE)_beginthread(&LThread::ThreadEntry, 0, this);
		if (!IsValid())
		{
			m_bState = false;
		}
		return m_bState;
	}

	bool IsValid()	//true 有效， false 异常
	{
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	bool Stop()
	{
		if (m_bState == false) return true;
		m_bState = false;
		bool ret = WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0;
		//if (m_worker.load() != NULL)
		//{
		//	::ThreadWorker* pWorker = m_worker.load();
		//	m_worker.store(NULL);
		//	delete pWorker;
		//}
		UpdateWorker();
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker())
	{
		/*m_worker.store(worker);*/
		if (!worker.IsValid())
		{
			m_worker.store(NULL);
			return;
		}
		if (m_worker.load() != NULL)
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	//true 表示已经空闲， false 表示还没有仍然再工作
	bool IsIdle()
	{
		return !m_worker.load()->IsValid();
	}


	////返回值小于0，则终止线程循环， 大于0 则警告日志
	//virtual int each_step() = 0;	//用户自定义

private:
	void ThreadWorker()
	{
		while (m_bState)
		{
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid())
			{
				int ret = worker();
				if (ret != 0)
				{
					CString str;
					//str.Format(_T("thread found warning code ! %d\r\n", ret));
					OutputDebugString(str);
				}
				if (ret < 0)
				{
					m_worker.store(NULL);
				}
			}
			else
			{
				Sleep(1);
			}
		}
	}

	static void ThreadEntry(void* arg)
	{
		LThread* thiz = (LThread*)arg;
		if (thiz)
		{
			thiz->ThreadWorker();
		}
		_endthread();
	}

private:
	HANDLE m_hThread;
	bool m_bState;
	std::atomic<::ThreadWorker*> m_worker;
};

class LThreadPool
{
public:
	LThreadPool(size_t size) 
	{
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
		{
			m_threads[i] = new LThread();
		}
	}
	LThreadPool() {}
	~LThreadPool(){
		Stop();
		m_threads.clear();
	}
	bool Invoke()
	{
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (!m_threads[i]->Start())
			{
				ret = false;
				break;
			}
		}
		if (ret == false)
		{
			Stop();
		}
		return true;
	}

	void Stop()
	{
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i]->Stop();
		}
	}

	//线程池分发worker
	int DispathWorker(const ThreadWorker& worker)
	{
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i]->IsIdle())
			{
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index)
	{
		if (index < m_threads.size())
		{
			return m_threads[index]->IsValid();
		}
		return false;
	}


private:
	std::mutex m_lock;
	std::vector<LThread*> m_threads;
};