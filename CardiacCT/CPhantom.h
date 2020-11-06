
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
public:

	CScrollBar		_scroll_bar;

	HDC				_hDC;				// MFC視窗DC
	CDIB*			_2D_dib;
	CRect			_2D_rect;
	CWnd*			_2D_frame;

///-------------------------↓ 2D影像 設定參數 ↓----------------------------------------///

	BYTE**			_img;				// 原始灰階資訊

	int				_display_slice;		// 顯示的切片
	int				_total_slice;		// 總張數
	int				_row;				// 影像(列)
	int				_col;				// 影像(行)

///-------------------------↑ 2D影像 設定參數 ↑----------------------------------------///



//================//
//   Operations   //
//================//
public:
	void*	new2Dmatrix(int h, int w, int size);	// 動態配置二維矩陣

	void	Draw2DImage(const int& slice);			// 繪製二維影像

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
	
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
