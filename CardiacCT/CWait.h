
// CWait.h : ����Y�ʧ@�ɪ����� Dialog
//

//====================//
//   Waiting Dialog   //
//====================//

#pragma once

class CWait : public CDialogEx
{
	DECLARE_DYNAMIC(CWait)

//================//
//   Attributes   //
//================//
public:

//================//
//   Operations   //
//================//
public:
	void		setDisplay(CString text);

//================//
// Implementation //
//================//
public:
	CWait(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CWait();

//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_WAIT };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};
