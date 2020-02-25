
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

	enum class ControlModes				// ����ާ@�Ҧ�
	{
		ControlNone,					// ���������ާ@
		ControlObject,					//  -��Ŧ- �ާ@
		ControlPlane,					//  -����- �ާ@
	}	mode;

//================//
//   Attributes   //
//================//
public:
	CCTDoc*			m_pDoc;				// ����Doc���������
	
	CScrollBar		m_ScrollBar;

	HDC				m_hDC;				// MFC����DC
	HGLRC			m_hRC;				// openGL����DC
	CDIB*			m_2D_dib;
	CRect			m_3D_rect;
	CRect			m_2D_rect;
	CWnd*			m_3D_frame;
	CWnd*			m_2D_frame;
	BOOL			gl_3DTexture;		// �P�_�O�_�䴩3D���z
PFNGLTEXIMAGE3DPROC glTexImage3D;		// Address of an openGL extension function.
	
	GLuint			textureName[5];		// �O�����z�W��
	
///-------------------------�� openGL �]�w�Ѽ� ��----------------------------------------///
	int				Mat_Offset;			// �v���x�}�m���������q
	int				ImageFrame;			// �v���ؼ�(1�Ӯɧ�1��)
	int				glSlices;			// openGL�إ߯��z�h��

	bool			LR_Button;			// true:�ƹ�����Ffalse:�ƹ��k��
	bool			Act_Rotate;
	bool			Act_Translate;

	float			scale_x;			// �i�H����ܤ�Ҫ����_�Ѽ�
	float			scale_y;
	float			scale_z;
	float			density;
	float			intensity;

	float*			obj_axis;			// ���� ���� ����b
	float*			pln_axis;			// ���� ���� ����b
	float			obj_angle;			// ���� ���� ���ਤ��(Rotation)
	float			pln_angle;			// ���� ���� ���ਤ��(Rotation)
	float			viewDistance;		// ���� ���u �첾(Translation)
	
	float			transY;
	float*			lastPos;
	float**			glVertexPt;			// openGLø���I
	double*			user_Plane;
///-------------------------�� openGL �]�w�Ѽ� ��----------------------------------------///

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///

	typedef struct
	{
		double x;
		double y;
		double z;
	}	Seed;

	Seed			seed_pt;			// �b2D�����I����seed�y��
	Seed			seed_gl;			// �b3D�����I����seed openGL�@�ɮy��
	Seed			seed_img;			// 3D_seed���@�ɮy���ഫ����v��"�x�}"(512*512)�y��

	bool			get_3Dseed;			// �O�_�I��F3D�ؤl�I
	bool			get_2Dseed;			// �O�_�I��F2D�ؤl�I
	bool			get_regionGrow;		// �O�_����L�ϰ즨��
	float			z_index;			// �ե�Z�b���Ѽ�

	BYTE**			judge;				// �ϰ즨���P�w

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///

	short			HUThreshold;		// �G�Ȥ��H��(HU)
unsigned short		PixelThreshold;		// �G�Ȥ��H��(pixel)
unsigned short		DisplaySlice;		// ��ܪ�slice(�q0�}�l)
	

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);					// �]�mhDC�����榡
	BOOL	ExtensionSupported(const char* exten);

	void	GLInitialization();							// openGL�غc��l��
	void	PerspectiveBuild();							// �إ߳z���Ŷ�
	void	PrepareVolume(unsigned int texName[10]);	// �إ߯��z�y�Ъ���Ưx�}
	void	getRamp(GLubyte* color, float t, int n);	// �W��

	void	Draw3DImage(bool which);					// ø�s�T���v��
	void	Draw2DImage(unsigned short &slice);			// ø�s�G���v��
	void*	new2Dmatrix(int h, int w, int size);		// �ʺA�t�m�G���x�}
	void*	new3Dmatrix(int h, int w, int l, int size);
	void*	new4Dmatrix(int h, int w, int l, int v, int size);

	Seed	coordiConvert(Seed &pt);					// openGL coordinate -> data array site
	void	InvertMat(float (&m)[16]);
	void	ActTracking(int x, int y);					// ����ާ@���l��(�]�t����P�첾)
	void	ActStop(UINT nFlags, int x, int y);			// �u��������v���ʧ@�]�w
	void	ActStart(UINT nFlags, int x, int y);		// �u�}�l����v���ʧ@�]�w
	void	pointToVector(int x, int y, int width, int height, float vec[3]);

	bool	Region_Growing(Seed &seed);					// �T�� �ϰ즨��
	
//================//
// Implementation //
//================//
public:
	C3DProcess(CWnd* pParent = nullptr);	// standard constructor
	virtual ~C3DProcess();
	virtual BOOL OnInitDialog();

//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_3DPROCESS };
#endif
public:										// CString�������A���bAttributes�t�~�]�ܼ��x�s
	BOOL		m_plane;
	BOOL		m_object;
	BOOL		m_complete;
	BOOL		m_thresholdHU;
	BOOL		m_thresholdPixel;

	BOOL		m_3Dseed;

	CString		m_pixelThreshold;
	CString		m_HUThreshold;
	CString		m_intensity;
	CString		m_density;
	CString		m_slices;
	CString		m_pos_1;
	CString		m_pos_2;
	CString		m_pos_3;
	CString		m_pos_4;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	
	afx_msg void OnBnClickedCheck3dSeed();
	afx_msg void OnBnClickedCheckPlane();
	afx_msg void OnBnClickedCheckObject();
	afx_msg void OnBnClickedCheckComplete();
	afx_msg void OnBnClickedCheckHuThreshold();
	afx_msg void OnBnClickedCheckPixelThreshold();

	afx_msg void OnEnChangeEditSlices();
	afx_msg void OnEnChangeEditHuThreshold();
	afx_msg void OnEnChangeEditPixelThreshold();

	afx_msg void OnBnClickedButton3dseedClear();
	afx_msg void OnBnClickedButtonSeedChange();
	afx_msg void OnBnClickedButtonSlicesPlus();
	afx_msg void OnBnClickedButtonSlicesMinus();
	afx_msg void OnBnClickedButtonDensityPlus();
	afx_msg void OnBnClickedButtonDensityMinus();
	afx_msg void OnBnClickedButtonIntensityPlus();
	afx_msg void OnBnClickedButtonIntensityMinus();
	afx_msg void OnBnClickedButtonRegionGrowing();
};
