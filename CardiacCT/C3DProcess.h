
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

#define _IN							
#define _OUT		

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
	int				Mat_Offset;			// 3D�v���x�}�m���������q
	int				ImageFrame;			// �v���ؼ�(1�Ӯɧ�1��)
	int				glSlices;			// openGL�إ߯��z�h��

	bool			LR_Button;			// true:�ƹ�����Ffalse:�ƹ��k��
	bool			Act_Rotate;
	bool			Act_Translate;
	bool			get_3Dimage;		// �O�_�w�g�إ� 3D image

	float			scale_x;			// �i�H����ܤ�Ҫ����_�Ѽ�
	float			scale_y;			// ���T���ƭȬ����д��ի�ҳ]�w��
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
		short x, y, z;
	}	Seed_s;

	typedef struct
	{
		double x, y, z;
	}	Seed_d;

	typedef struct
	{
		_IN		Seed_s	seed;			// �ؤl�I
		_IN		int		kernel;			// ���U�ȥ��O���_��
		_IN		int		z_upLimit;		// Z�b�����W��(�̦h�� 0)
		_IN		int		z_downLimit;	// Z�b�����U��(�̦h�� TotalSlice)
		_IN		double	threshold;		// �����H��
		_OUT	double	growingVolume;	// ������n
	}	RG_Factor;

	RG_Factor		RG_Total;			// �ϰ즨������P���p
	Seed_d			seed_gl;			// �b3D�����I����seed openGL�@�ɮy��
	Seed_s			seed_pt;			// �b2D�����I����seed�y��
	Seed_s			seed_img;			// 3D_seed���@�ɮy���ഫ����v��"�x�}"(512*512)�y��

	bool			get_3Dseed;			// �O�_�I��F3D�ؤl�I
	bool			get_2Dseed;			// �O�_�I��F2D�ؤl�I
	bool			get_regionGrow;		// �O�_����L�ϰ즨��

	float			x_index;			// �ե� X �b��Ҫ��Ѽ�
	float			y_index;			// �ե� Y �b��Ҫ��Ѽ�
	float			z_index;			// �ե� Z �b��Ҫ��Ѽ�

	BYTE**			judge;				// �ϰ즨���P�w
	BYTE			m_image0[256*256*256][4];

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///

	short			HUThreshold;		// �G�Ȥ��H��(HU)
unsigned short		PixelThreshold;		// �G�Ȥ��H��(pixel)
unsigned short		DisplaySlice;		// ��ܪ�slice(�q0�}�l)

///------------- �� ����� �� -------------///



///------------- �� ����� �� -------------///

//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);					// �]�mhDC�����榡
	BOOL	ExtensionSupported(const char* exten);

	void	LoadVolume();								// �إ߯��z
	void	PrepareVolume();							// �إ߯��z�y�Ъ���Ưx�}
	void	GLInitialization();							// openGL�غc��l��
	void	PerspectiveBuild();							// �إ߳z���Ŷ�
	void	getRamp(GLubyte* color, float t, int n);	// �W��

	void	Draw3DImage(bool which);					// ø�s�T���v��
	void	Draw2DImage(unsigned short& slice);			// ø�s�G���v��
	void*	new2Dmatrix(int h, int w, int size);		// �ʺA�t�m�G���x�}
	void*	new3Dmatrix(int h, int w, int l, int size);
	void*	new4Dmatrix(int h, int w, int l, int v, int size);

	void	InvertMat(float (&m)[16]);
	void	ActTracking(int x, int y);					// ����ާ@���l��(�]�t����P�첾)
	void	ActStop(UINT nFlags, int x, int y);			// �u��������v���ʧ@�]�w
	void	ActStart(UINT nFlags, int x, int y);		// �u�}�l����v���ʧ@�]�w
	void	pointToVector(int x, int y, int width, int height, float vec[3]);

	void	Region_Growing_3D(RG_Factor& factor);		// 3D �ϰ즨��(�������G�s��judge)
	Seed_s	coordiConvert(Seed_d& pt);					// openGL coordinate -> data array site

///------------- �� ����� �� -------------///

	void	Erosion_3D(); 

///------------- �� ����� �� -------------///
	
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
	BOOL		m_3Dseed;
	BOOL		m_plane;
	BOOL		m_object;
	BOOL		m_complete;
	BOOL		m_thresholdHU;
	BOOL		m_thresholdPixel;

	CString		m_pixelThreshold;
	CString		m_HUThreshold;
	CString		m_intensity;
	CString		m_density;
	CString		m_slices;
	CString		m_result;
	CString		m_pos_1;
	CString		m_pos_2;
	CString		m_pos_3;
	CString		m_pos_4;
	CString		m_pos_5;
	CString		m_pos_6;
	CString		m_pos_7;
	CString		m_pos_8;

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

	afx_msg void OnBnClickedButton2dseedClear();
	afx_msg void OnBnClickedButton3dseedClear();
	afx_msg void OnBnClickedButtonPlaneReset();
	afx_msg void OnBnClickedButtonSeedChange();
	afx_msg void OnBnClickedButtonSlicesPlus();
	afx_msg void OnBnClickedButtonSlicesMinus();
	afx_msg void OnBnClickedButtonDensityPlus();
	afx_msg void OnBnClickedButtonDensityMinus();
	afx_msg void OnBnClickedButtonIntensityPlus();
	afx_msg void OnBnClickedButtonIntensityMinus();
	afx_msg void OnBnClickedButtonRegionGrowing();
	afx_msg void OnBnClickedButtonGrowingClear();
};
