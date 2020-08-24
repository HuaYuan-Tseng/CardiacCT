
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

	unsigned int		m_Step;				// �C�]�@�����B��
	unsigned int		m_Range;			// �i�ױ�����
	unsigned int		m_Cur_pos;			// �{�b����m

//================//
//   Operations   //
//================//
public:
	void	Run();
	void	SetStatic(CString str);						// �]�w��r
	void	SetPosition(int index);						// �]�w��m
	void	SetInitial(int pos, int step, int range);	// �]�w�i�ת��׻P��l��m

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
