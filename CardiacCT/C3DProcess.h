
// C3DProcess.h : interface of the C3DProcess class
//

//==========================//
//   3D Processing Dialog   //
//==========================//

#pragma once
#include "CDIB.h"
#include "CCTDoc.h"
#include "glut.h"
#include "glext.h"

class C3DProcess : public CDialogEx
{
	DECLARE_DYNAMIC(C3DProcess)

//================//
//   Attributes   //
//================//
public:
	CCTDoc*			m_pDoc;				// 紀錄Doc的物件指標
	
	CScrollBar		m_ScrollBar;

	CWnd*			m_2D_frame;
	CWnd*			m_3D_frame;
	CRect			m_2D_rect;
	CRect			m_3D_rect;
	CDIB*			m_2D_dib;
	HGLRC			m_hRC;				// openGL視窗DC
	HDC				m_hDC;				// MFC視窗DC

	BOOL			gl_3DTexture;		// 判斷是否支援3D紋理
PFNGLTEXIMAGE3DPROC glTexImage3D;		// Address of an openGL extension function.
	
	GLuint			textureName[10];	// 記載紋理名稱

	int				Mat_Offset;			// 影像矩陣置中的偏移量
	int				ImageFrame;			// 影像框數(1個時序1個)
	float**			glVertexPt;			// openGL繪圖點
unsigned short		DisplaySlice;		// 顯示的slice(從0開始)
	

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);				// 設置hDC像素格式
	BOOL	ExtensionSupported(const char* exten);

	void	GLInitialization();						// openGL建構初始化
	void	PerspectiveBuild();						// 建立透視空間
	void	Draw2DImage(unsigned short &slice);		// 繪製二維影像
	void*	new2Dmatrix(int h, int w, int size);	// 動態配置二維矩陣
	void	LoadVolume(unsigned int texName[10]);

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
