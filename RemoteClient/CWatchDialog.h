﻿#pragma once
#include "afxdialogex.h"


// CWatchDialog 对话框

class CWatchDialog : public CDialog
{
	DECLARE_DYNAMIC(CWatchDialog)

public:
	CWatchDialog(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CWatchDialog();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIG_WATCH };
#endif

public:
	int m_nObjWidth;
	int m_nObjHeight;
	CImage m_image;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

	bool m_isFull;	//缓存是否有数据， true 代表 有缓存数据，

public:

	void SetImageStatus(bool isFull = false)
	{
		m_isFull = isFull;
	}

	bool isFull() const {
		return m_isFull;
	}

	CImage& GetImage()
	{
		return m_image;
	}


	CPoint UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen = false);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CStatic m_picture;
	virtual BOOL OnInitDialog();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnStnClickedWatch();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	virtual void OnOK();
	afx_msg void OnBnClickedBtnLock();
	afx_msg void OnBnClickedBtnUnlock();
};
