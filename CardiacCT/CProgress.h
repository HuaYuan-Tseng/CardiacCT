
// CProgress.h : 進度條 Dialog
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
	CProgressCtrl		m_ProgressBar;		// 進度條物件

//================//
//   Operations   //
//================//
public:
	void	SetStatic(CString str);			// 設定文字
	void	Set(int range, int pos);		// 設定進度長度與初始位置
	void	GetPro(int index);				// 設定位置

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
