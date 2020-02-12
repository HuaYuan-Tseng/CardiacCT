
// C3DProcess.h : interface of the C3DProcess class
//

//==========================//
//   3D Processing Dialog   //
//==========================//

#pragma once
#include "CDIB.h"
#include "CCTDoc.h"
#include "glew.h"
#include "freeglut.h"

class C3DProcess : public CDialogEx
{
	DECLARE_DYNAMIC(C3DProcess)

	// �������Ҧ�
	//
	enum MoveModes 
	{
		MoveNone, MoveView, MoveObject, MoveTexture
	};
	enum MoveModes mode;

//================//
//   Attributes   //
//================//
public:
	CCTDoc*			m_pDoc;				// ����Doc���������
	
	CScrollBar		m_ScrollBar;

	CWnd*			m_2D_frame;
	CWnd*			m_3D_frame;
	CRect			m_2D_rect;
	CRect			m_3D_rect;
	CDIB*			m_2D_dib;
	HGLRC			m_hRC;				// openGL����DC
	HDC				m_hDC;				// MFC����DC

	BOOL			gl_3DTexture;		// �P�_�O�_�䴩3D���z
PFNGLTEXIMAGE3DPROC glTexImage3D;		// Address of an openGL extension function.
	
	GLuint			textureName[10];	// �O�����z�W��
	GLboolean		redrawContinued;
	GLboolean		trackingMotion;
	GLboolean		trackingTranslation;

	int				Mat_Offset;			// �v���x�}�m���������q
	int				ImageFrame;			// �v���ؼ�(1�Ӯɧ�1��)
	int				slices;
	int				lastTime;

	bool			gbPlane;
	bool			gbPlaneMove;
	bool			resetPlane;
	bool			savePlane;
	bool			loadangle;
	bool			m_object;
	bool			m_plane;

	float*			axis;				// ���� ���� ����b
	float*			pAxis;				// ���� ���� ����b
	float			angle;				// ���� ���� ���ਤ��(Rotation)
	float			pAngle;				// ���� ���� ���ਤ��(Rotation)
	float			viewDistance;		// ���� ���u �첾(Translation)
	float**			glVertexPt;			// openGLø���I
	float*			planeangle;
	float*			planeset;
	float*			lastPos;
	float*			Xform;
	float			transPosY;
	float			scale_x;			// �i�H����ܤ�Ҫ����_�Ѽ�
	float			scale_y;
	float			scale_z;
	float			density;
	float			intensity;
	
	double*			user;

unsigned short		DisplaySlice;		// ��ܪ�slice(�q0�}�l)
	

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);					// �]�mhDC�����榡
	BOOL	ExtensionSupported(const char* exten);

	void	GLInitialization();							// openGL�غc��l��
	void	PerspectiveBuild();							// �إ߳z���Ŷ�
	void	InvertMat(float m[16]);
	void	Draw3DImage(BOOL which);					// ø�s�T���v��
	void	Draw2DImage(unsigned short &slice);			// ø�s�G���v��
	void*	new2Dmatrix(int h, int w, int size);		// �ʺA�t�m�G���x�}
	void	PrepareVolume(unsigned int texName[10]);	// �إ߯��z�y�Ъ���Ưx�}
	void	getRamp(GLubyte* color, float t, int n);	// �W��

	void	TrackMotion(int x, int y);
	void	StopMotion(int x, int y, int time);
	void	StartMotion(int x, int y, int time);
	void	pointToVector(int x, int y, int width, int height, float vec[3]);

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
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
