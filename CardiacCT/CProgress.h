
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

	unsigned int		m_Step;				// 每跑一次的步數
	unsigned int		m_Range;			// 進度條長度
	unsigned int		m_Cur_pos;			// 現在的位置

//================//
//   Operations   //
//================//
public:
	void	Run();
	void	SetStatic(CString str);						// 設定文字
	void	SetPosition(int index);						// 設定位置
	void	SetInitial(int pos, int step, int range);	// 設定進度長度與初始位置

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
