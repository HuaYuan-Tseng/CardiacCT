
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
	ON_BN_CLICKED(IDC_BUTTON_PHANTOM_OPEN, &CPhantom::OnBnClickedButtonPhantomOpen)
END_MESSAGE_MAP()

//===============================//
//   CPhantom message handlers   //
//===============================//

BOOL CPhantom::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	// Dialog 初始化的事件
	//

	/* 更換Dialog標題 */
	SetWindowTextA("Phantom Verify");



	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPhantom::OnBnClickedButtonPhantomOpen()
{
	// TODO: Add your control notification handler code here
	// Button : Open (Open Phantom Folder)
	//



}

