
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

void CProgress::SetInitial(int pos, int step, int range)
{
	// DO : �]�w�i�ױ����׻P��l��m
	//
	m_ProgressBar.SetRange(0, range);
	m_ProgressBar.SetPos(pos);
	m_Cur_pos = pos;
	m_Range = range;
	m_Step = step;
}

void CProgress::SetStatic(CString str)
{
	//  DO : �]�w��r
	// 
	CClientDC pDC(this);
	pDC.SetBkMode(TRANSPARENT);
	pDC.TextOut(15, 10, str);
	pDC.UpdateColors();
}

void CProgress::Run()
{
	m_Cur_pos += m_Step;
	m_ProgressBar.SetPos(m_Cur_pos);
	m_ProgressBar.UpdateWindow();
}

void CProgress::SetPosition(int index)
{
	// DO : �]�w�i�ױ���m
	//
	m_ProgressBar.SetPos(index);
	m_ProgressBar.UpdateWindow();
}


