// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "stdint.h"
#include <list>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "Command.h"
#include <conio.h>
#include "LQueue.h"

CWinApp theApp;
using namespace std;

/*
CString strPath = CString(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")); //32位程序
*/
//#define INVOKE_PATH _T("C:\\Windows\\System32\\RemoteCtrl.exe");
#define INVOKE_PATH _T("C:\\Users\\10977\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")

void writeRegister(const CString& strPath)
{
    CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    char sPath[MAX_PATH] = "";
    char sSys[MAX_PATH] = "";
    std::string strExe = "\\RemoteCtrl.exe";
    GetCurrentDirectoryA(MAX_PATH, sPath);
    GetSystemDirectoryA(sSys, sizeof(sSys));

    std::string strCmd = "mklink " + std::string(sSys) + strExe + " "
        + std::string(sPath) + strExe;
    int ret = system(strCmd.c_str());
    //向注册表填写软链接
    HKEY hKey = NULL;
    /*
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);
    */
    ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_WRITE, &hKey);
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("开机启动设置失败"), _T("失败"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    // 不使用环境变量 自动展开路径
    //TCHAR sSysPath[MAX_PATH] = _T("");
    //GetSystemDirectoryW(sSysPath, MAX_PATH);
    //CString strPath = sSysPath + CString(_T("\\RemoteCtrl.exe"));
    //ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

    //使用环境变量自动 自动展开路径
    //CString strPath = CString(_T("%SystemRoot%\\system32\\RemoteCtrl.exe"));
    ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
    if (ret != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        MessageBox(NULL, _T("开机启动设置失败"), _T("失败"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
    RegCloseKey(hKey);
}

//copy file -> to currency file directory(静态库)


/*
如果 vs 编译出 32位程序
*/
bool ChooseAutoInvoke(const CString& strPath)
{
    //C:\Users\Lintao\AppData\Roaming\Microsoft\Windows\Start Menu\Programs
    if (PathFileExists(strPath))
    {
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\r\n");
    strInfo += _T("继续运行该程序，将使得这台机器用于被控状态");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        //终端命令：拷贝 exe 的软链接
        if (!Tool::WriteStartupDir(strPath))
        {
            MessageBox(NULL, _T("复制文件夹失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
            return false;
        }
    }
    else if (ret == IDCANCEL)
    { 
        return false;
    }
    return true;
}

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH  1
#define IOCP_LIST_POP   2

enum {
    IocpListEmpty,
    IocpListPush,
    IocpListPop
};

typedef struct IocpParam
{
    int nOperator;                  //操作
    std::string strData;            //数据
    _beginthread_proc_type cbFunc;  //回调
    IocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL)
    {
        nOperator = op;
        strData   = sData;
        cbFunc = cb;
    }
    IocpParam()
    {
        nOperator = -1;
    }
}IOCP_PARAM;


// Input/Output Completion Port
HANDLE hIOCP = INVALID_HANDLE_VALUE;
void threadMain(HANDLE hIOCP)
{
    std::list<std::string> lstString;

    DWORD dwTransferred = 0;
    ULONG_PTR CompletionKey = 0;
    OVERLAPPED* pOverlapped = NULL;
    int countPush = 0;
    int countPop = 0;
    while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
    {
        if ((dwTransferred == 0) || (CompletionKey == NULL))
        {
            //printf("thread is prepare to exit!\r\n");
            break;
        }
        IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
        if (pParam->nOperator == IocpListPush)
        {
            lstString.push_back(pParam->strData);
            //std::cout << "push : lstString size = " << lstString.size() << std::endl;
            countPush++;
        }
        else if (pParam->nOperator == IocpListPop)
        {
            std::string* pStr = NULL;
            if (lstString.size() > 0)
            {
                pStr = new std::string(lstString.front());
                lstString.pop_front();
            }
            if (pParam->cbFunc)
            {
                pParam->cbFunc(pStr);
            }
            countPop++;
        }
        else if (pParam->nOperator == IocpListEmpty) {
            lstString.clear();
        }
        delete pParam;
    }

    std::cout << "thread: " << "coutPush = " << countPush << " , "
        << "coutPop = " << countPop << std::endl;
}



void threadQueueEntry(HANDLE hIOCP)
{
    threadMain(hIOCP);
    _endthread();
}

void func(void* arg)
{
    std::string* pstr = (std::string*)arg;
    if (pstr != NULL)
    {
        printf("pop from list: %s\r\n", pstr->c_str());
        delete pstr;
    }
    else
    {
        printf("list is empty, no data !\r\n");
    }
}



int main()
{
    if (!Tool::Init()) return 1;
    for (int i = 0; i < 10; i++)
    {
        printf("i = %d\r\n", i);

    }
    //::exit(0);





    /*
    HANDLE hIOCP = INVALID_HANDLE_VALUE;
    hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    if (hIOCP == INVALID_HANDLE_VALUE || (hIOCP == NULL))
    {
        printf("create iocp failed!\r\n", GetLastError());
        return 1;
    }
    HANDLE hThread = (HANDLE)_beginthread(threadQueueEntry, 0, hIOCP);
    std::cout << "press any key to quit!\r\n";
    
    ULONGLONG tick1 = GetTickCount64();
    ULONGLONG tick2 = GetTickCount64();
    int countPop = 0, countPush = 0;
    while (_kbhit() == 0)
    {
        if (GetTickCount64() - tick2 > 1300)
        {
            BOOL RET = PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world", func), NULL);
            tick2 = GetTickCount64();
            countPop++;
        }
        if (GetTickCount64() - tick1 > 2000)
        {
            PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
            tick1 = GetTickCount64();
            countPush++;
        }
        Sleep(1);
    }
    if (hIOCP != NULL)
    {
        //TODO:唤醒完成端口
        PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
        WaitForSingleObject(hThread, INFINITE);
    }
    CloseHandle(hIOCP);
    printf("exit done!\r\n");

    std::cout << "main: " << "coutPush = " << countPush << " , "
        << "coutPop = " << countPop << std::endl;

    ::exit(0);

    */

    /*
    if (Tool::IsAdmin())
    {
        if (!Tool::Init()) return -1;
        TRACE("RUN AS ADMINISTRATOR!\r\n");
        MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
        
        if (ChooseAutoInvoke(INVOKE_PATH))
        {
            CCommand cmdHandle;
            CServerSocket* pserver = CServerSocket::getInstance();
            int ret = pserver->Run(&CCommand::RunCommand, &cmdHandle);
            switch (ret)
            {
            case -1:
            {
                MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            }
            break;

            case -2:
            {
                MessageBox(NULL, _T("多次无法正常接入用户， 结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            }
            break;
            }
        }
    }
    else
    {
        TRACE("RUN AS NORMAL!\r\n");
        MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
        if (Tool::RunAsAdmin() == false)
        {
            Tool::ShowError();
            return 1;
        }
    }
    */

   
    return 0;
}
