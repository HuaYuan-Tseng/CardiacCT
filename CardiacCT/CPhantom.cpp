
// CPhantom.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "CPhantom.h"
#include "afxdialogex.h"


//=======================//
//   3D Phantom Dialog   //
//=======================//

IMPLEMENT_DYNAMIC(CPhantom, CDialogEx)

CPhantom::CPhantom(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPHANTOM, pParent)
{

}

CPhantom::~CPhantom()
{
}

void CPhantom::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPhantom, CDialogEx)
END_MESSAGE_MAP()

//===============================//
//   CPhantom message handlers   //
//===============================//


