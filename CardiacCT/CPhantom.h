
// CPhantom.h : interface of the CPhantom class
//

//=======================//
//   3D Phantom Dialog   //
//=======================//

#pragma once
#include "CDIB.h"
#include "glew.h"
#include "freeglut.h"

class CPhantom : public CDialogEx
{
	DECLARE_DYNAMIC(CPhantom)
	
//================//
//   Attributes   //
//================//




//================//
//   Operations   //
//================//



//================//
// Implementation //
//================//
public:
	CPhantom(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPhantom();


//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_3DPHANTOM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonPhantomOpen();
	virtual BOOL OnInitDialog();
};
