// CWatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "ClientSocket.h"
#include "ClientController.h"


// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIG_WATCH, pParent),
	m_nObjWidth(-1),
	m_nObjHeight(-1),
	m_isFull(false)
{
}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
	ON_WM_LBUTTONDBLCLK()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
END_MESSAGE_MAP()


BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);
	return TRUE;  // return TRUE unless you set the focus to a control

}

//目前我需要根据 服务器端的鼠标操作封装 CwaychDialog 的鼠标操作
CPoint CWatchDialog::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	//客户端的大小是800 x 450
	if (!isScreen)
	{
		ClientToScreen(&point);
	}
	m_picture.ScreenToClient(&point);
	//else
	//{
	//	ClientToScreen(&point);
	//	m_picture.ScreenToClient(&point);
	//}
	//TRACE("point x=%d, y=%d\r\n", point.x, point.y);
	CRect clientRect;
	m_picture.GetWindowRect(clientRect);
	auto width0 = clientRect.Width();
	auto height0 = clientRect.Height();
	//TRACE("width0 = %d\r\n, height0 = %d\r\n", clientRect.Width(), clientRect.Height());
	int width = m_nObjWidth;
	int height = m_nObjHeight;
	//TRACE("width1 = %d\r\n, height1 = %d\r\n", width, height);
	
	return CPoint(point.x * m_nObjWidth / clientRect.Width(),
				  point.y * m_nObjHeight / clientRect.Height());
}

// CWatchDialog 消息处理程序
void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*) GetParent();
		CClientController* pParent = (CClientController*) GetParent();
		if (m_isFull)
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, SRCCOPY);
			//CImage image;
			//pParent->getImage(image);

			m_nObjHeight = m_image.GetHeight();
			m_nObjWidth = m_image.GetWidth();
			//if (m_nObjHeight != pParent->GetImage().GetHeight() || m_nObjWidth == pParent->GetImage().GetWidth())
			//{
			//	m_nObjHeight = pParent->GetImage().GetHeight();
			//	m_nObjWidth = pParent->GetImage().GetWidth();
			//}
			m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,
											rect.Width(), rect.Height());
			//TRACE("getimage width : %d, height : %d\r\n", m_nObjHeight, m_nObjWidth);
			

			TRACE("更新图片完成 %d %d %08X\r\n", m_nObjWidth, m_nObjHeight,
				(HBITMAP)m_image);


			m_picture.InvalidateRect(NULL);
			m_image.Destroy();
			m_isFull = false;
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 0;
		event.nAction = 1;

		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);

		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));



	}
	
	CDialog::OnLButtonDblClk(nFlags, point);
}

void dumpPacket(const CPacket& pack)
{
	//int len = pack.Size();
	std::string strOut;
	pack.Data(strOut);

	const char* packData = strOut.c_str();
	std::string showStr = "client send packet: ";
	for (int i = 0; i < strOut.size(); i++)
	{
		char buf[8] = "";
		snprintf(buf, sizeof(buf), "%02X ", packData[i] & 0xFF);
		showStr += buf;
	}
	OutputDebugString(showStr.c_str());
}

void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 0;
		event.nAction = 2;
		//CPacket packet(5, (BYTE*)&event, sizeof(event));
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);

		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 0;
		event.nAction = 3;
		//CPacket packet(5, (BYTE*)&event, sizeof(event));
		//CClientSocket::getInstance().Send(packet);
		// 
		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);
	
		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnLButtonUp(nFlags, point);
}




void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 1;
		event.nAction = 1;

		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);


		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 1;
		event.nAction = 2;

		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);

		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 1;
		event.nAction = 3;
		//CPacket packet(5, (BYTE*)&event, sizeof(event));
		//CClientSocket::getInstance().Send(packet);

		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);


		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_nObjHeight != -1 && m_nObjWidth != -1)
	{
		CPoint remotePoint = UserPoint2RemoteScreenPoint(point);
		//TRACE("x, y %d, %d\r\n", remotePoint.x, remotePoint.y);
		MOUSEEV event;
		event.ptXY = remotePoint;
		event.nButton = 4;
		event.nAction = 4;

		//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) & event);


		CClientController* pController = CClientController::getInstance();
		pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));


		//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

	}

	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	TRACE("OnStnClickedWatch() \r\n");
	CPoint point;
	GetCursorPos(&point);

	CPoint remotePoint = UserPoint2RemoteScreenPoint(point, true);
	MOUSEEV event;
	event.ptXY = remotePoint;
	event.nButton = 0;
	event.nAction = 0;
	TRACE("x, y %d, %d\r\n", remotePoint.x, remotePoint.y);
	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//pParent->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (LPARAM) &event);

	CClientController* pController = CClientController::getInstance();
	pController->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

	//CClientController::getInstance().SendCommandPacket(5, true, (BYTE*)&event, sizeof(event));

}




void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}





void CWatchDialog::OnBnClickedBtnLock()
{


	CClientController* pController = CClientController::getInstance();
	pController->SendCommandPacket(GetSafeHwnd(), 7);
	//CClientController::getInstance().SendCommandPacket(7);

	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//pParent->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{

	CClientController* pController = CClientController::getInstance();
	pController->SendCommandPacket(GetSafeHwnd(), 8);
	//CClientController::getInstance().SendCommandPacket(8);


	//CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
	//pParent->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}
