#pragma once

#include <iostream>
#include <map>
#include <direct.h>
#include "ServerSocket.h"
#include "Tool.h"
#include "io.h"
#include <direct.h>
#include <atlimage.h>
#include "LockInfoDialog.h"
#include "resource1.h"
#include <list>

class CCommand
{
public:
	CCommand();
    ~CCommand() {};
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstPacket, CPacket& inPacket);
    static void RunCommand(void* arg, int status, std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        CCommand* thiz = (CCommand*)arg;
        if (status > 0)
        {
            int ret = thiz->ExcuteCommand(status, lstPacket, inPacket);
            if (ret != 0)
            {
                TRACE("执行命令失败！ %d ret = %d\r\n", status, ret);
            }
        }
        else
        {
            MessageBox(NULL, _T("无法正常接入用户，自动重试"), _T("接入用户失败"), MB_OK | MB_ICONERROR);
        }
    }

protected:
    CLockDialog dlg;
    unsigned threadid;
	//成员函数指针
    //typedef int(CCommand::* CMDFUNC)();    //c 风格写法
	using CMDFUNC = int(CCommand::*)(std::list<CPacket>&, CPacket&);      //C++
	//成员函数映射表
	std::map<int, CMDFUNC> m_mapFunction;
	//成员函数命令
	using cmdFunctionMap = 
	struct
	{
		int nCmd;
		CMDFUNC func;
	};
protected:
    
    //command 1:查看磁盘分区
    int MakeDriverInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string result;
        for (int i = 1; i <= 26; i++)
        {
            if (_chdrive(i) == 0)
            {
                if (result.size() > 0)
                {
                    result += ',';
                }
                result += 'A' + i - 1;  //A软盘 ， 
            }
        }
        lstPacket.emplace_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
        //CPacket p(1, (BYTE*)result.c_str(), result.size());
        //Tool::Dump((BYTE*)p.Data(), p.Size());
        //CServerSocket::getInstance()->Send(p);
        return 0;
    }

    //command 2:查看指定目录下的文件
    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        //std::string strPath ;
   /*     std::list<FILEINFO> lsFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
        {
            OutputDebugString(_T("当前的命令，不是获取文件列表"));
            return -1;
        }*/
        if (_chdir(strPath.c_str()) != 0)
        {
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            //memcpy(finfo.szFileName, strPath.c_str(), strPath.size());
            //CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            OutputDebugString(_T("没有权限，访问目录！！"));
            return -2;
        }

        _finddata_t fdata;
        intptr_t hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1)
        {
            OutputDebugString(_T("没有找到任何文件！"));
            FILEINFO finfo;
            finfo.HasNext = FALSE;
            //CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            return -3;
        }

        int count = 0;
        long sendTotalSize = 0;
        bool isSendSc;
        do {
            FILEINFO finfo;
            finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;
            memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
            //CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
            //isSendSc = CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
            //TRACE("%s send state : %d, len = %d\r\n", finfo.szFileName, isSendSc, sizeof(finfo));
            sendTotalSize += sizeof(finfo);
            count++;
        } while (!_findnext(hfind, &fdata));

        FILEINFO finfo;
        finfo.HasNext = FALSE;
        //CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
        //isSendSc = CServerSocket::getInstance()->Send(pack);
        lstPacket.emplace_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

        sendTotalSize += sizeof(finfo);
        //TRACE("%s send state : %d, len = %d\r\n", finfo.szFileName, isSendSc, sizeof(finfo));
        //TRACE("Server send total size = %ld\r\n", sendTotalSize);
        //TRACE("SERVER HAS SEND COMMAND 2 PACKET!!!\r\n");
        return 0;
    }

    //command 3:打开文件
    int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        //CServerSocket::getInstance()->GetFilePath(strPath);
        ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

        //CPacket pack(3, NULL, 0);
        //CServerSocket::getInstance()->Send(pack);

        lstPacket.emplace_back(CPacket(3, NULL, 0));
        return 0;
    }

    //command 4: 下载文件
    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        //CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;

        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");   //使用 fopen_s 代替 fopen

        if (err != 0)
        {
            //CPacket pack(4, (BYTE*)&data, 8);
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }

        if (pFile != NULL)
        {
            fseek(pFile, 0, SEEK_END);          //这里为什么要把光标挪到文件末尾
            data = _ftelli64(pFile);
            //CPacket head(4, (BYTE*)&data, 8);
            //CServerSocket::getInstance()->Send(head);
            lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
            fseek(pFile, 0, SEEK_SET);
            char buffer[1024] = "";

            size_t rlen = 0;
            do
            {
                rlen = fread(buffer, 1, 1024, pFile);
                //CPacket pack(4, (BYTE*)buffer, rlen);
                //CServerSocket::getInstance()->Send(pack);
                lstPacket.emplace_back(CPacket(4, (BYTE*)buffer, rlen));
            } while (rlen >= 1024);
            fclose(pFile);
        }
        //CPacket pack(4, NULL, 0);
        //CServerSocket::getInstance()->Send(pack);
        lstPacket.emplace_back(CPacket(4, NULL, 0));
        return 0;
    }

    //command 5: 鼠标事件
    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

            DWORD nFlags = 0;
            switch (mouse.nButton) {
            case 0://左键
                nFlags = 1;
                break;
            case 1://右键
                nFlags = 2;
                break;
            case 2://中键
                nFlags = 4;
                break;
            case 4://没有按键
                nFlags = 8;
                break;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction)
            {
            case 0://单击
                nFlags |= 0x10;
                break;
            case 1://双击
                nFlags |= 0x20;
                break;
            case 2://按下
                nFlags |= 0x40;
                break;
            case 3://放开
                nFlags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags)
            {
            case 0x21://左键双击
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://左键单击
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://左键按下

                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://左键放开
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://右键双击
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://右键单击
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://右键按下
                TRACE("右键按下！\r\n");
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://右键放开
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://中键双击
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://中键单击
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://中键按下
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://中键放开
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08://单纯的鼠标移动
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            //CPacket pack(4, NULL, 0);
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(4, NULL, 0));
        

        return 0;
    }

    //command 6: 截图
    int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {

        CImage screen;//GDI
        HDC hScreen = ::GetDC(NULL);
        int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//24   ARGB8888 32bit RGB888 24bit RGB565  RGB444
        int nWidth = GetDeviceCaps(hScreen, HORZRES);
        int nHeight = GetDeviceCaps(hScreen, VERTRES);
        screen.Create(nWidth, nHeight, nBitPerPixel);
        //TRACE("server width : %d, height : %d\r\n", nWidth, nHeight);
        BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
        ReleaseDC(NULL, hScreen);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
        if (hMem == NULL)return -1;
        IStream* pStream = NULL;
        HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
        if (ret == S_OK) {
            screen.Save(pStream, Gdiplus::ImageFormatPNG);
            LARGE_INTEGER bg = { 0 };
            pStream->Seek(bg, STREAM_SEEK_SET, NULL);
            PBYTE pData = (PBYTE)GlobalLock(hMem);
            SIZE_T nSize = GlobalSize(hMem);
            //CPacket pack(6, pData, nSize);
            //CServerSocket::getInstance()->Send(pack);
            TRACE("nWidth = %d, nHeight = %d , nSize = %d\r\n", nWidth, nHeight, nSize);
            lstPacket.emplace_back(CPacket(6, pData, nSize));
            GlobalUnlock(hMem);
        }
        pStream->Release();
        GlobalFree(hMem);
        screen.ReleaseDC();
        return 0;
    }

    //command 7: 锁机
    static unsigned __stdcall threadLockDlg(void* arg)
    {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);    // _endthreadex 是和
        return 0;
    }

    void threadLockDlgMain()
    {
        TRACE(" %s(%d): %d \r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dlg.Create(IDD_DIALOG_INFO, NULL);

        dlg.ShowWindow(SW_SHOW);
       

        //遮蔽后台窗口
        CRect rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
        rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        rect.bottom = LONG(rect.bottom * 1.10);
        dlg.MoveWindow(rect);
        CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
        if (pText)
        {
            CRect rtText;
            pText->GetWindowRect(rtText);
            int nWidth = rtText.Width();
            int x = (rect.right - nWidth) / 2;
            int nHeight = rtText.Height();
            int y = (rect.bottom - nHeight) / 2;
            pText->MoveWindow(x, y, nWidth, nHeight);
        }

        dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
        //dlg.GetWindowRect(rect);
        //TRACE("right : %lu bottom: %lu \r\n", rect.right, rect.bottom);
        // 
        //限制鼠标的范围
        rect.right = rect.left + 1;
        rect.bottom = rect.top + 1;
        ShowCursor(false);                                                //隐藏鼠标
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);   //关闭 windows 任务栏
        ClipCursor(rect);
        //限制鼠标的范围

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_KEYDOWN)
            {
                TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
                if (msg.wParam == 0x1b)
                {
                    break;
                }
            }
        }
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
        ShowCursor(true);

        dlg.DestroyWindow();
    }

    int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE)
        {
            //_beginthread(threadLockDlg, 0, NULL);
            _beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
            TRACE(" LockMachine : threadid = %d\r\n", threadid);
        }
        
        //CPacket pack(7, NULL, 0);
        //CServerSocket::getInstance()->Send(pack);
        lstPacket.emplace_back(CPacket(7, NULL, 0));
        
        return 0;
    }

    //command 8:解锁
    int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //dlg.SendMessage(WM_KEYDOWN, 0X1B, 0X00010001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0X1B, 0X00010001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0X1B, 0X00010001);
        return 0;
    }

    //command 9: 删除本地文件
    int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //TODO: 
        std::string strPath = inPacket.strData;
        //CServerSocket::getInstance()->GetFilePath(strPath);
        TCHAR sPath[MAX_PATH] = _T("");

        size_t num_chars;
        errno_t err = mbstowcs_s(&num_chars, sPath, MAX_PATH, strPath.c_str(), strlen(strPath.c_str()));
        DeleteFile(sPath);

  /*      CPacket pack(9, NULL, 0);
        bool ret = CServerSocket::getInstance()->Send(pack);*/
        lstPacket.emplace_back(CPacket(9, NULL, 0));

        return 0;
    }

    //command 1981: 测试连接
    int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //CPacket pack(1981, NULL, 0);
        //CServerSocket::getInstance()->Send(pack);
        lstPacket.emplace_back(CPacket(1981, NULL, 0));
        return 0;
    }
};

