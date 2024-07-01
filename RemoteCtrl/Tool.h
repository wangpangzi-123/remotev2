#pragma once
class Tool
{
public:
	static void Dump(BYTE* pData, size_t nSize)	//应该加入 static 
	{
		std::string strOut = "Packet: ";
		for (int i = 0; i < nSize; i++)
		{
			char hexData[8];
			snprintf(hexData, sizeof(hexData), " %02X ", pData[i] & 0xFF);
			strOut += hexData;
		}
		OutputDebugStringA(strOut.c_str());
	}

    static void ShowError()
    {
        LPWSTR lpMessageBuf = NULL;
        //strerror(errno);    // std 标准库error
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)&lpMessageBuf, 0, NULL);
        OutputDebugString(lpMessageBuf);
        LocalFree(lpMessageBuf);
    }

	static bool IsAdmin()
    {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            ShowError();
            return false;
        }
        TOKEN_ELEVATION eve{};
        DWORD len = 0;
        if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE)
        {
            ShowError();
            return false;
        }
        CloseHandle(hToken);
        if (len == sizeof(eve))
        {
            return eve.TokenIsElevated;
        }
        return false;
    }

    static bool RunAsAdmin()
    {

        TRACE(L"LOGON ADMIN SUCCESS!\r\n");
        STARTUPINFO si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        TCHAR sPath[MAX_PATH] = _T("");
        GetModuleFileName(NULL, sPath, MAX_PATH);
        BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
        if (!ret)
        {
            ShowError();
            MessageBox(NULL, _T("创建进程失败"), _T("程序错误"), 0);
            return false;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
};

