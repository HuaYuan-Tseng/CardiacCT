
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

	int				Mat_Offset;			// �v���x�}�m���������q
	int				ImageFrame;			// �v���ؼ�(1�Ӯɧ�1��)
	float**			glVertexPt;			// openGLø���I
unsigned short		DisplaySlice;		// ��ܪ�slice(�q0�}�l)
	

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);				// �]�mhDC�����榡
	BOOL	ExtensionSupported(const char* exten);

	void	GLInitialization();						// openGL�غc��l��
	void	PerspectiveBuild();						// �إ߳z���Ŷ�
	void	Draw2DImage(unsigned short &slice);		// ø�s�G���v��
	void*	new2Dmatrix(int h, int w, int size);	// �ʺA�t�m�G���x�}
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
