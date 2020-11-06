
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

	HDC				_hDC;				// MFC����DC
	CDIB*			_2D_dib;
	CRect			_2D_rect;
	CWnd*			_2D_frame;

///-------------------------�� 2D�v�� �]�w�Ѽ� ��----------------------------------------///

	BYTE**			_img;				// ��l�Ƕ���T

	int				_display_slice;		// ��ܪ�����
	int				_total_slice;		// �`�i��
	int				_row;				// �v��(�C)
	int				_col;				// �v��(��)

///-------------------------�� 2D�v�� �]�w�Ѽ� ��----------------------------------------///



//================//
//   Operations   //
//================//
public:
	void*	new2Dmatrix(int h, int w, int size);	// �ʺA�t�m�G���x�}

	void	Draw2DImage(const int& slice);			// ø�s�G���v��

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
