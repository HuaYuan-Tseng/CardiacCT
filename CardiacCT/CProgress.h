
// CProgress.h : �i�ױ� Dialog
//

//=========================//
//   Progress Bar Dialog   //
//=========================//

#pragma once

class CProgress : public CDialogEx
{
	DECLARE_DYNAMIC(CProgress)

//================//
//   Attributes   //
//================//
public:
	CProgressCtrl		m_ProgressBar;		// �i�ױ�����

//================//
//   Operations   //
//================//
public:
	void	SetStatic(CString str);			// �]�w��r
	void	Set(int range, int pos);		// �]�w�i�ת��׻P��l��m
	void	GetPro(int index);				// �]�w��m

//================//
// Implementation //
//================//
public:
	CProgress(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CProgress();

//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_PROGRESSBAR };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	
};
