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

void WriteStartupDir(const CString& strPath)
{

    CString strCmd = GetCommandLine();
    strCmd.Replace(_T("\""), _T(""));
    BOOL ret = CopyFile(strCmd, strPath, FALSE);

    if (ret == FALSE)
    {
        MessageBox(NULL, _T("复制文件夹失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
        exit(0);
    }
}

/*
如果 vs 编译出 32位程序
*/
bool ChooseAutoInvoke(const CString& strPath)
{

    //C:\Users\Lintao\AppData\Roaming\Microsoft\Windows\Start Menu\Programs
    if (PathFileExists(strPath))
    {
        return;
    }
    CString strInfo = _T("该程序只允许用于合法的用途！\r\n");
    strInfo += _T("继续运行该程序，将使得这台机器用于被控状态");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES)
    {
        //终端命令：拷贝 exe 的软链接
        WriteStartupDir(strPath);
    }
    else if (ret == IDCANCEL)
    { 
        return false;
    }
    return true;
}


bool Init()
{
    HMODULE hModule = ::GetModuleHandle(nullptr);
    if (hModule == nullptr)
    {
        wprintf(L"错误: GetModuleHandle 失败\n");
        return false;
    }
    if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
    {
        wprintf(L"错误: MFC 初始化失败\n");
        return false;
    }
    return true;
}

int main()
{
    if (Tool::IsAdmin())
    {
        if (!Init()) return -1;
        TRACE("RUN AS ADMINISTRATOR!\r\n");
        MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
        CCommand cmdHandle;
        ChooseAutoInvoke(INVOKE_PATH);
        CServerSocket* pserver = CServerSocket::getInstance();
        int ret = pserver->Run(&CCommand::RunCommand, &cmdHandle);
        switch (ret)
        {
        case -1:
        {
            MessageBox(NULL, _T("网络初始化异常"), _T("网络初始化失败"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        break;

        case -2:
        {
            MessageBox(NULL, _T("多次无法正常接入用户， 结束程序！"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
            exit(0);
        }
        break;
        }
    }
    else
    {
        TRACE("RUN AS NORMAL!\r\n");
        MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);
        if (Tool::RunAsAdmin() == false)
        {
            Tool::ShowError();
        }
        return 0;
    }

    return 0;
}
