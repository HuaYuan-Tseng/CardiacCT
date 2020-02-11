
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

	// 物件旋轉模式
	//
	enum class MoveModes 
	{
		MoveNone, MoveView, MoveObject, MoveTexture
	};
	enum class MoveModes mode = MoveModes::MoveNone;

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
	int				slices;

	bool			gbPlane;
	bool			gbPlaneMove;
	bool			resetPlane;
	bool			savePlane;
	bool			loadangle;

	float*			axis;				// 控制 物件 旋轉軸
	float*			pAxis;				// 控制 物件 旋轉軸
	float			angle;				// 控制 物件 旋轉角度(Rotation)
	float			pAngle;				// 控制 平面 旋轉角度(Rotation)
	float			viewDistance;		// 控制 視線 位移(Translation)
	float**			glVertexPt;			// openGL繪圖點
	float*			planeangle;
	float*			planeset;
	float*			Xform;
	float			scale_x;			// 可以改顯示比例的神奇參數
	float			scale_y;
	float			scale_z;
	float			density;
	float			intensity;
	
	double*			user;

unsigned short		DisplaySlice;		// 顯示的slice(從0開始)
	

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);					// 設置hDC像素格式
	BOOL	ExtensionSupported(const char* exten);

	void	GLInitialization();							// openGL建構初始化
	void	PerspectiveBuild();							// 建立透視空間
	void	InvertMat(float* mat);
	void	Draw3DImage(BOOL which);					// 繪製三維影像
	void	Draw2DImage(unsigned short &slice);			// 繪製二維影像
	void*	new2Dmatrix(int h, int w, int size);		// 動態配置二維矩陣
	void	PrepareVolume(unsigned int texName[10]);
	void	getRamp(GLubyte* color, float t, int n);	// 上色

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
