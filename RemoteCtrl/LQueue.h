#pragma once

#include <mutex>
#include <atomic>
#include <iostream>
#include <list>
#include "pch.h"
#include "LThread.h"


//线程安全队列（IOCP实现）
template<class T>
class LQueue
{
public:
    enum {
        EQNone,
        EQPush,
        EQPop,
        EQSize,
        EQClear
    };
    typedef struct IocpParam
    {
        int nOperator;                  //操作
        T Data;                         //数据
        HANDLE hEvent;
        IocpParam(int op, const T& data, HANDLE hEve = NULL)
        {
            nOperator = op;
            Data = data;
            hEvent = hEve;
        }
        IocpParam()
        {
            nOperator = EQNone;
        }
    }PPARAM;//Post Parameter
public:
    LQueue()
        :  m_hThread(INVALID_HANDLE_VALUE),
           m_lock(false)
    {
        m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
        if (m_hCompletionPort != NULL)
        {
            m_hThread = (HANDLE)_beginthread(&LQueue<T>::threadEntry, 0, this);
        }
    }
    virtual ~LQueue()
    {
        if (m_lock) return;
        m_lock = true;
        
        PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
        WaitForSingleObject(m_hThread, INFINITE);
        if (m_hCompletionPort != NULL)
        {
            HANDLE hTemp = m_hCompletionPort;
            m_hCompletionPort = NULL;
            CloseHandle(hTemp);
        }
    }

    bool PushBack(const T& data)
    {
        if (m_lock) return false;
        IocpParam* pParam = new IocpParam(EQPush, data);
        bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
        (ULONG_PTR)pParam, NULL);
        if (ret == false) delete pParam;
        return ret;
    }

    virtual bool PopFront(T& data)
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        IocpParam Param(EQPop, data, hEvent);
        if (m_lock)
        {
            if (hEvent) CloseHandle(hEvent);
            return false;
        }
        bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
            (ULONG_PTR)&Param, NULL);
        if (ret == false) {
            CloseHandle(hEvent);
            return false;
        }
        ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
        if (ret)
        {
            data = Param.Data;
        }
        return ret;
    }

    size_t Size()
    {
        HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        IocpParam Param(EQSize, T(), hEvent);
        if (m_lock)
        {
            if (hEvent) CloseHandle(hEvent);
            return -1;
        }
        bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
            (ULONG_PTR)&Param, NULL);
        if (ret == false) {
            CloseHandle(hEvent);
            return -1;
        }
        ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
        if (ret)
        {
            return Param.nOperator;
        }
        return -1;
    }

    bool Clear()
    {
        if (m_lock) return false;
        IocpParam* pParam = new IocpParam(EQClear, T());
        bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
            (ULONG_PTR)pParam, NULL);
        if (ret == false) delete pParam;
        return ret;
    }

protected:
    static void threadEntry(void* arg)
    {
        LQueue<T>* thiz = (LQueue<T>*)arg;
        thiz->threadMain();
        _endthread();
    }

    virtual void dealParam(PPARAM* pParam)
    {
        switch (pParam->nOperator)
        {
        case EQPush:
        {
            m_lstData.push_back(pParam->Data);
            delete pParam;
        }
        break;

        case EQPop:
        {
            if (m_lstData.size() > 0)
            {
                pParam->Data = m_lstData.front();
                m_lstData.pop_front();
            }
            if (pParam->hEvent != NULL)
            {
                SetEvent(pParam->hEvent);
            }
        }
        break;

        case EQSize:
        {
            pParam->nOperator = m_lstData.size();
            if (pParam->hEvent != NULL)
            {
                SetEvent(pParam->hEvent);
            }
        }
        break;

        case EQClear:
        {
            m_lstData.clear();
            delete pParam;
        }
        break;

        default:
            OutputDebugStringA("unknown operator!\r\n");
            break;
        }
    }

    void threadMain()
    {
        PPARAM* pParam = NULL;
        DWORD dwTransferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* pOverlapped = NULL;

        while (GetQueuedCompletionStatus(
            m_hCompletionPort, 
            &dwTransferred, 
            &CompletionKey, 
            &pOverlapped, 
            INFINITE))
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL))
            {
                //printf("thread is prepare to exit!\r\n");
                break;
            }
            pParam = (PPARAM*)CompletionKey;
            dealParam(pParam);
        }
        while (GetQueuedCompletionStatus(
            m_hCompletionPort,
            &dwTransferred,
            &CompletionKey,
            &pOverlapped,
            0))
        {
            if ((dwTransferred == 0) || (CompletionKey == NULL))
            {
                //printf("thread is prepare to exit!\r\n");
                continue;
            }
            pParam = (PPARAM*)CompletionKey;
            dealParam(pParam);
        }
        HANDLE hTemp = m_hCompletionPort;
        m_hCompletionPort = NULL;
        CloseHandle(hTemp);
    }

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
    std::atomic<bool> m_lock;
};


//class ThreadFuncBase;
//typedef int (ThreadFuncBase::* FUNCTYPE)();

template<class T>
class LSendQueue : public LQueue<T>, ThreadFuncBase
{
public:
    typedef int (ThreadFuncBase::* LCALLBACK)(T& data);

    LSendQueue(ThreadFuncBase* obj, LCALLBACK callback)
        : LQueue<T>(), m_base(obj), m_callback(callback)
    {
        m_thread.Start();
        m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&LSendQueue<T>::threadTick));
    }

    virtual ~LSendQueue() {
        
        m_base = NULL;
        m_callback = NULL;
        m_thread.Stop();
    }
//    virtual bool PopFront(T& data) = delete;

protected:
    virtual bool PopFront(T& data) {
        return false;
    }
    bool PopFront()
    {
        typename LQueue<T>::IocpParam* Param = new typename LQueue<T>::IocpParam(LQueue<T>::EQPop, T());
        if (LQueue<T>::m_lock)
        {
            delete Param;
            return false;
        }

        bool ret = PostQueuedCompletionStatus(LQueue<T>::m_hCompletionPort, sizeof(*Param), (ULONG_PTR)&Param, NULL);
        if (ret == false) {
            delete Param;
            return false;
        }
        return ret;
    }

    int threadTick()
    {
        if (WaitForSingleObject(LQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)
        {
            return -1;
        }
        if (LQueue<T>::m_lstData.size() > 0)
        {
            PopFront();
        }
        return 0;
    }

    virtual void dealParam(typename LQueue<T>::PPARAM* pParam)
    {
        switch (pParam->nOperator)
        {
        case LQueue<T>::EQPush:
        {
            LQueue<T>::m_lstData.push_back(pParam->Data);
            delete pParam;
        }
        break;

		case LQueue<T>::EQPop:
		{
			if (LQueue<T>::m_lstData.size() > 0)
			{
				pParam->Data = LQueue<T>::m_lstData.front();
				if ((m_base->*m_callback)(pParam->Data) == 0)
				{
                    LQueue<T>::m_lstData.pop_front();
				}
			}
			delete pParam;
		}
		break;

        case LQueue<T>::EQSize:
        {
            pParam->nOperator = LQueue<T>::m_lstData.size();
            if (pParam->hEvent != NULL)
            {
                SetEvent(pParam->hEvent);
            }
        }
        break;

        case LQueue<T>::EQClear:
        {
            LQueue<T>::m_lstData.clear();
            delete pParam;
        }
        break;

        default:
            OutputDebugStringA("unknown operator!\r\n");
            break;
        }
    }

private:
    ThreadFuncBase* m_base;
    LCALLBACK m_callback;
    LThread m_thread;
};


typedef LSendQueue<std::vector<char>>::LCALLBACK SENDCALLBACK;