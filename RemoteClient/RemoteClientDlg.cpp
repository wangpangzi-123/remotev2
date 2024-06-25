
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include <thread>
#include "ClientSocket.h"
#include <string>
#include <sstream>

#include "ClientController.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "CWatchDialog.h"

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框



class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
	, m_watchIsClose(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE1, m_Tree);
	DDX_Control(pDX, IDC_LIST1, m_List);
}

void Dump_lt(const char* pData, size_t nSize)
{
	std::string strOut = "client : ";

	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xff);
		strOut += buf;
	}
	OutputDebugStringA(strOut.c_str());
}

//int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
//{
//	//return CClientController::getInstance().SendCommandPacket(nCmd, bAutoClose, pData, nLength);
//	
//	CClientController* pController = CClientController::getInstance();
//	return pController->SendCommandPacket(nCmd, bAutoClose, pData, nLength);
//
//}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()


	ON_BN_CLICKED(IDC_BUTTON2, &CRemoteClientDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE1, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST1, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	//ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0xC0A8D582; //C0A8D502 0x7F000001 0xC0A8D582
	m_nPort = _T("9527");

	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	//CClientController::getInstance().UpdateAddress(m_server_address,
	//	atoi((LPCTSTR)m_nPort));

	UpdateData(FALSE);
	m_dlgStatus.Create(IDD_DIG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedButton2()
{
	TRACE("OnBnClickedButton2 \r\n");
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1981);
}



//点下查看 文件目录的按钮 就能够 查看对方（被控机的目录文件）
void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	/*int ret = SendCommandPacket(1);*/
	std::list<CPacket> lstPackets;	
	CClientController* pController = CClientController::getInstance();
	//int ret = CClientController::getInstance().SendCommandPacket(1);
	int ret = pController->SendCommandPacket(GetSafeHwnd(), 1, true, NULL, 0);
	if (ret == -1 || (lstPackets.size() <= 0))
	{
		AfxMessageBox(_T("命令处理失败！！！"));
		return;
	}
}



void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	//int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength()); //send command packet
	CClientController* pController = CClientController::getInstance();

	std::list<CPacket> lstPackets;
	int nCmd = pController->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());

	//int nCmd = CClientController::getInstance().SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	
	
	while (lstPackets.size() > 0)
	{
		std::list<CPacket>::iterator it = lstPackets.begin();
		for (; it != lstPackets.end(); it++)
		{
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
			if (pInfo->HasNext)
			{
				if (pInfo->IsDirectory == false)
				{
					m_List.InsertItem(0, pInfo->szFileName);
				}
			}
		}
	}


	//PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance().getPacket().strData.c_str();

	//while (pInfo->HasNext)
	//{
	//	//TRACE("lt:   [%s] is dir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
	//	if (pInfo->IsDirectory == false)
	//	{
	//		m_List.InsertItem(0, pInfo->szFileName);
	//	}
	//	int cmd = CClientSocket::getInstance().dealCommand();
	//	TRACE("ack : %d\r\n", cmd);
	//	if (cmd < 0) break;
	//	pInfo = (PFILEINFO)CClientSocket::getInstance().getPacket().strData.c_str();
	//}

	//CClientSocket::getInstance().closeClient();
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
		return;
	if (m_Tree.GetChildItem(hTreeSelected) == NULL)
		return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);
	//int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength()); //send command packet
	CClientController* pController = CClientController::getInstance();
	std::list<CPacket> lstPackets;
	int nCmd = pController->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(), (WPARAM)hTreeSelected);

	if (lstPackets.size() > 0)
	{
		std::list<CPacket>::iterator it = lstPackets.begin();
		for (; it != lstPackets.end(); it++)
		{
			/*PFILEINFO pInfo = (PFILEINFO)(*it->strData.c_str());*/
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
			if (pInfo->HasNext)
			{
				if (pInfo->IsDirectory)
				{
					if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))
					{
						continue;
					}
					TRACE("directory name: %s\r\n", pInfo->szFileName);
					HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
					m_Tree.InsertItem("", hTemp, TVI_LAST);
				}
				else
				{
					m_List.InsertItem(0, pInfo->szFileName);
				}
			}
		}
	}
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do
	{
		strTmp = m_Tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_Tree.GetParentItem(hTree);
	} while (hTree != NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	HTREEITEM hSub = NULL;
	do
	{
		hSub = m_Tree.GetChildItem(hTree);
		if(hSub != NULL) 
			m_Tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);	//屏幕坐标转化成为 客户端的坐标
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPopup = menu.GetSubMenu(0);
	if (pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this);
	}
}

//

void CRemoteClientDlg::OnDownloadFile()
{
	int nListSelected   = m_List.GetSelectionMark();
	CString strFile     = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	CClientController* pController = CClientController::getInstance();
	//int ret = CClientController::getInstance().downFile(strFile);

	int ret = pController->downFile(strFile);

	if (ret != 0)
	{
		MessageBox(_T("下载失败！"));
		TRACE("下载失败 ret = %d\r\n", ret);
	}
}

//TODO:大文件传输需要额外的处理



void CRemoteClientDlg::OnDeleteFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	//int ret = SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
	CClientController* pController = CClientController::getInstance();
	int ret = pController->SendCommandPacket(GetSafeHwnd(), 9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	//int ret = CClientController::getInstance().SendCommandPacket(9, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0)
	{
		AfxMessageBox("删除文件命令执行失败！");
	}
	LoadFileCurrent();//文件夹中有文件显示， 
}


void CRemoteClientDlg::OnRunFile()
{
	HTREEITEM hSelected = m_Tree.GetSelectedItem();	//拿到节点
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	//拿到文件名
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	TRACE("strFile = %s\r\n", strFile);

	CClientController* pController = CClientController::getInstance();
	int ret = pController->SendCommandPacket(GetSafeHwnd(), 3, true, (BYTE*)(LPCSTR)strFile, strFile.GetLength());

	if (ret < 0)
	{
		AfxMessageBox("打开文件失败！\r\n");
	}
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2))
	{
		//错误处理

	}
	else if (lParam == 1)
	{
		//对方关闭套接字

	}
	else
	{
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL)
		{
			CPacket& head = *pPacket;
			switch (pPacket->sCmd)
			{
			case 1:
			{
				std::string drivers = head.strData;
				std::string dr;
				m_Tree.DeleteAllItems();
				OutputDebugString(drivers.c_str());
				TRACE("drivers.size() = %d\r\n", drivers.size());

				size_t pos = 0;
				while ((pos = drivers.find(',')) != std::string::npos)
				{
					dr = drivers.substr(0, pos);
					TRACE("dr = %s\r\n", dr.c_str());
					dr += ":";
					HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
					m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
					drivers.erase(0, pos + 1);
				}
				dr = drivers.substr(0, 1);
				dr += ":";
				HTREEITEM hTemp = m_Tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
				m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
			}
			break;

			case 2:
			{
				PFILEINFO pInfo = (PFILEINFO)(head).strData.c_str();
				if (pInfo->HasNext)
				{
					if (pInfo->IsDirectory)
					{
						if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))
						{
							break;
						}
						TRACE("directory name: %s\r\n", pInfo->szFileName);
						HTREEITEM hTemp = m_Tree.InsertItem(pInfo->szFileName, (HTREEITEM)lParam, TVI_LAST);
						m_Tree.InsertItem("", hTemp, TVI_LAST);
					}
					else
					{
						m_List.InsertItem(0, pInfo->szFileName);
					}
				}
			}
			break;
			case 3:
				TRACE("run file done!\r\n");
				break;
			case 4:
			{
				static LONGLONG length = 0, index = 0;
				if (length == 0)
				{
					length = *(long long*)head.strData.c_str();
					if (length == 0)
					{
						AfxMessageBox("文件长度为0 或者 无法读取文件！！！");
						CClientController::getInstance()->DownloadEnd();
						break;
					}
				}
				else if (length > 0 && index >= length)
				{
					fclose((FILE*)lParam);
					length = 0;
					index = 0;
					CClientController::getInstance()->DownloadEnd();
				}
				else
				{
					FILE* pFile = (FILE*)lParam;
					size_t actuallyWriteBytes = fwrite(head.strData.c_str(),1 ,head.strData.size(), pFile);
					index += actuallyWriteBytes;
				}
			}
			break;
			case 9:
				TRACE("delete file done!\r\n");
				break;
			case 1981:
				TRACE("test connected success!\r\n");
				break;
			default:
				TRACE("unknow data received! %d\r\n", head.sCmd);
				break;
			}
		}
	}
	return 0;
}

//LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
//{
//	int ret = 0;
//	int cmd = wParam >> 1;
//	CClientController* pController = CClientController::getInstance();
//	switch (cmd)
//	{
//		case 4:
//		{
//			CString strFile = (LPCSTR)lParam;
//			
//			//ret = CClientController::getInstance().SendCommandPacket(cmd, wParam & 1,
//			//	(BYTE*)(LPCSTR)strFile, strFile.GetLength());
//
//			ret = pController->SendCommandPacket(cmd, wParam & 1, (BYTE*)(LPCSTR)strFile, strFile.GetLength());
//		}
//		break;
//
//		case 5:
//		{
//			//ret = CClientController::getInstance().SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
//			ret = pController->SendCommandPacket(cmd, wParam & 1,
//				(BYTE*)lParam, sizeof(MOUSEEV));
//		}
//		break;
//	
//		case 6:
//		case 7:
//		case 8:
//		{
//			//ret = SendCommandPacket(cmd, wParam & 1, NULL, 0);
//			//ret = CClientController::getInstance().SendCommandPacket(cmd, wParam & 1, NULL, 0);
//			ret = pController->SendCommandPacket(cmd, wParam & 1, NULL, 0);
//		}
//		break;
//		
//		default:
//			ret = -1;
//	}
//
//	return ret;
//}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	CClientController* pController = CClientController::getInstance();
	pController->startWatch();
	//CClientController::getInstance().startWatch();
}


//void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
//{
//	// TODO: 在此添加消息处理程序代码和/或调用默认值
//
//	CDialogEx::OnTimer(nIDEvent);
//}



void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	
	//CClientController::getInstance().UpdateAddress(m_server_address,
	//	atoi((LPCTSTR)m_nPort));
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	UpdateData();
	CClientController* pController = CClientController::getInstance();
	
	pController->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	//CClientController::getInstance().UpdateAddress(m_server_address,
	//	atoi((LPCTSTR)m_nPort));
}
