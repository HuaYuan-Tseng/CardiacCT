
// C3DProcess.h : interface of the C3DProcess class
//

//==========================//
//   3D Processing Dialog   //
//==========================//

#pragma once
#include "CDIB.h"
#include "CCTDoc.h"

class C3DProcess : public CDialogEx
{
	DECLARE_DYNAMIC(C3DProcess)

//================//
//   Attributes   //
//================//
public:
	CCTDoc*			m_pDoc;					// 紀錄Doc的物件指標

	CWnd*			m_2D_frame;
	CRect			m_2D_rect;
	CDIB*			m_2D_dib;
	CScrollBar		m_ScrollBar;
	
	unsigned short		DisplaySlice;		// 顯示的slice(從0開始)

//================//
//   Operations   //
//================//
public:
	void	Draw2DImage(unsigned short &slice);		// 繪製二維影像
	void*	new2Dmatrix(int h, int w, int size);	// 動態配置二維矩陣

//================//
// Implementation //
//================//
public:
	C3DProcess(CWnd* pParent = nullptr);   // standard constructor
	virtual ~C3DProcess();
	virtual BOOL OnInitDialog();

//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_3DPROCESS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
