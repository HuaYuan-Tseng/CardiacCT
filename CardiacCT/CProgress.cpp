
// CProgress.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "CProgress.h"
#include "afxdialogex.h"

//=========================//
//   Progress Bar Dialog   //
//=========================//

IMPLEMENT_DYNAMIC(CProgress, CDialogEx)

CProgress::CProgress(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_PROGRESSBAR, pParent)
{
}

CProgress::~CProgress()
{
}

void CProgress::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROGRESS1, m_ProgressBar);
}

BEGIN_MESSAGE_MAP(CProgress, CDialogEx)
END_MESSAGE_MAP()

//================================//
//   CProgress message handlers   //
//================================//

void CProgress::GetPro(int index)
{
	// DO : 設定進度條位置
	//
	m_ProgressBar.SetPos(index);
	m_ProgressBar.UpdateWindow();
}

void CProgress::Set(int range, int pos)
{
	// DO : 設定進度條長度與初始位置
	//
	m_ProgressBar.SetRange(0, range);
	m_ProgressBar.SetPos(pos);
}

void CProgress::SetStatic(CString str)
{
	//  DO : 設定文字
	// 
	CClientDC pDC(this);
	pDC.SetBkMode(TRANSPARENT);
	pDC.TextOut(15, 10, str);
	pDC.UpdateColors();
}
