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
                TRACE("ִ������ʧ�ܣ� %d ret = %d\r\n", status, ret);
            }
        }
        else
        {
            MessageBox(NULL, _T("�޷����������û����Զ�����"), _T("�����û�ʧ��"), MB_OK | MB_ICONERROR);
        }
    }

protected:
    CLockDialog dlg;
    unsigned threadid;
	//��Ա����ָ��
    //typedef int(CCommand::* CMDFUNC)();    //c ���д��
	using CMDFUNC = int(CCommand::*)(std::list<CPacket>&, CPacket&);      //C++
	//��Ա����ӳ���
	std::map<int, CMDFUNC> m_mapFunction;
	//��Ա��������
	using cmdFunctionMap = 
	struct
	{
		int nCmd;
		CMDFUNC func;
	};
protected:
    
    //command 1:�鿴���̷���
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
                result += 'A' + i - 1;  //A���� �� 
            }
        }
        lstPacket.emplace_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
        //CPacket p(1, (BYTE*)result.c_str(), result.size());
        //Tool::Dump((BYTE*)p.Data(), p.Size());
        //CServerSocket::getInstance()->Send(p);
        return 0;
    }

    //command 2:�鿴ָ��Ŀ¼�µ��ļ�
    int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        //std::string strPath ;
   /*     std::list<FILEINFO> lsFileInfos;
        if (CServerSocket::getInstance()->GetFilePath(strPath) == false)
        {
            OutputDebugString(_T("��ǰ��������ǻ�ȡ�ļ��б�"));
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
            OutputDebugString(_T("û��Ȩ�ޣ�����Ŀ¼����"));
            return -2;
        }

        _finddata_t fdata;
        intptr_t hfind = 0;
        if ((hfind = _findfirst("*", &fdata)) == -1)
        {
            OutputDebugString(_T("û���ҵ��κ��ļ���"));
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

    //command 3:���ļ�
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

    //command 4: �����ļ�
    int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        std::string strPath = inPacket.strData;
        //CServerSocket::getInstance()->GetFilePath(strPath);
        long long data = 0;
        FILE* pFile = NULL;

        errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");   //ʹ�� fopen_s ���� fopen

        if (err != 0)
        {
            //CPacket pack(4, (BYTE*)&data, 8);
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(4, (BYTE*)&data, 8));
            return -1;
        }

        if (pFile != NULL)
        {
            fseek(pFile, 0, SEEK_END);          //����ΪʲôҪ�ѹ��Ų���ļ�ĩβ
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

    //command 5: ����¼�
    int MouseEvent(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        MOUSEEV mouse;
        memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

            DWORD nFlags = 0;
            switch (mouse.nButton) {
            case 0://���
                nFlags = 1;
                break;
            case 1://�Ҽ�
                nFlags = 2;
                break;
            case 2://�м�
                nFlags = 4;
                break;
            case 4://û�а���
                nFlags = 8;
                break;
            }
            if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
            switch (mouse.nAction)
            {
            case 0://����
                nFlags |= 0x10;
                break;
            case 1://˫��
                nFlags |= 0x20;
                break;
            case 2://����
                nFlags |= 0x40;
                break;
            case 3://�ſ�
                nFlags |= 0x80;
                break;
            default:
                break;
            }
            TRACE("mouse event : %08X x %d y %d\r\n", nFlags, mouse.ptXY.x, mouse.ptXY.y);
            switch (nFlags)
            {
            case 0x21://���˫��
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x11://�������
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x41://�������

                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x81://����ſ�
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x22://�Ҽ�˫��
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x12://�Ҽ�����
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x42://�Ҽ�����
                TRACE("�Ҽ����£�\r\n");
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x82://�Ҽ��ſ�
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x24://�м�˫��
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            case 0x14://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x44://�м�����
                mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x84://�м��ſ�
                mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
                break;
            case 0x08://����������ƶ�
                mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
                break;
            }
            //CPacket pack(4, NULL, 0);
            //CServerSocket::getInstance()->Send(pack);
            lstPacket.emplace_back(CPacket(4, NULL, 0));
        

        return 0;
    }

    //command 6: ��ͼ
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

    //command 7: ����
    static unsigned __stdcall threadLockDlg(void* arg)
    {
        CCommand* thiz = (CCommand*)arg;
        thiz->threadLockDlgMain();
        _endthreadex(0);    // _endthreadex �Ǻ�
        return 0;
    }

    void threadLockDlgMain()
    {
        TRACE(" %s(%d): %d \r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
        dlg.Create(IDD_DIALOG_INFO, NULL);

        dlg.ShowWindow(SW_SHOW);
       

        //�ڱκ�̨����
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
        //�������ķ�Χ
        rect.right = rect.left + 1;
        rect.bottom = rect.top + 1;
        ShowCursor(false);                                                //�������
        ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);   //�ر� windows ������
        ClipCursor(rect);
        //�������ķ�Χ

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

    //command 8:����
    int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //dlg.SendMessage(WM_KEYDOWN, 0X1B, 0X00010001);
        PostThreadMessage(threadid, WM_KEYDOWN, 0X1B, 0X00010001);
        //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0X1B, 0X00010001);
        return 0;
    }

    //command 9: ɾ�������ļ�
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

    //command 1981: ��������
    int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
    {
        //CPacket pack(1981, NULL, 0);
        //CServerSocket::getInstance()->Send(pack);
        lstPacket.emplace_back(CPacket(1981, NULL, 0));
        return 0;
    }
};

