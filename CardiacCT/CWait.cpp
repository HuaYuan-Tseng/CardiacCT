
// CWait.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "CWait.h"
#include "afxdialogex.h"

//===================//
//  Waiting Dialog   //
//===================//

IMPLEMENT_DYNAMIC(CWait, CDialogEx)

CWait::CWait(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_WAIT, pParent)
{
	display = _T("");
}

CWait::~CWait()
{
	if (display.IsEmpty() != true)
		display.Empty();
}

void CWait::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWait, CDialogEx)
END_MESSAGE_MAP()

//================================//
//   CProgress message handlers   //
//================================//

void CWait::setDisplay(CString text)
{
	CClientDC pDC(this);
	pDC.SetBkMode(TRANSPARENT);
	pDC.TextOut(15, 10, text);
	pDC.UpdateColors();
	display = text;
}