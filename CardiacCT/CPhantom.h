
// CPhantom.h : interface of the CPhantom class
//

//=======================//
//   3D Phantom Dialog   //
//=======================//

#pragma once
#include "CDIB.h"
#include "glew.h"
#include "freeglut.h"

#define _IN
#define _OUT

class CPhantom : public CDialogEx
{
	DECLARE_DYNAMIC(CPhantom)
	
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

	CScrollBar		_scroll_bar;

	HDC				_hDC;				// MFC����DC
	HGLRC			_hRC;				// openGL����DC
	CDIB*			_2D_dib;
	CRect			_2D_rect;
	CRect			_3D_rect;
	CWnd*			_2D_frame;
	CWnd*			_3D_frame;

	BOOL			gl_3DTexture;		// �P�_�O�_�䴩3D���z
	PFNGLTEXIMAGE3DPROC glTexImage3D;	// Address of an openGL extension function.

	GLuint			_textureName[5];	// �O�����z�W��

///-------------------------�� 2D�v�� �]�w�Ѽ� ��----------------------------------------///

	BYTE**			_img;				// ��l�Ƕ���T

	bool			_get_2D_image;
	int				_display_slice;		// ��ܪ�����
	int				_total_slice;		// �`�i��
	int				_row;				// �v��(�C)
	int				_col;				// �v��(��)

///-------------------------�� 2D�v�� �]�w�Ѽ� ��----------------------------------------///

///-------------------------�� openGL �]�w�Ѽ� ��----------------------------------------///
	int				_mat_offset;		// 3D�v���x�}�m���������q
	int				_image_frame;		// �v���ؼ�(1�Ӯɧ�1��)
	int				_gl_slices;			// openGL�إ߯��z�h��

	bool			_LR_button;			// true:�ƹ�����Ffalse:�ƹ��k��
	bool			_act_rotate;
	bool			_act_translate;
	bool			_get_3D_image;		// �O�_�w�g�إ� 3D image
	bool			_get_GL_build;		// �O�_�w�إ�openGL������

	float			_scale_x;			// �i�H����ܤ�Ҫ����_�Ѽ�
	float			_scale_y;			// ���T���ƭȬ����д��ի�ҳ]�w��
	float			_scale_z;
	float			_density;
	float			_intensity;

	float*			_obj_axis;			// ���� ���� ����b
	float*			_pln_axis;			// ���� ���� ����b
	float			_obj_angle;			// ���� ���� ���ਤ��(Rotation)
	float			_pln_angle;			// ���� ���� ���ਤ��(Rotation)
	float			_view_distance;		// ���� ���u �첾(Translation)

	float			_transY;
	float*			_last_pos;
	float**			_glVertexPt;		// openGLø���I
	double*			_user_plane;
///-------------------------�� openGL �]�w�Ѽ� ��----------------------------------------///

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///

	struct Seed_s
	{
		short x, y, z;
		Seed_s() : x(0), y(0), z(0) {};
		Seed_s(short a, short b, short c) : x(a), y(b), z(c) {};
	};

	struct Seed_d
	{
		double x, y, z;
		Seed_d() : x(0), y(0), z(0) {};
		Seed_d(double a, double b, double c) : x(a), y(b), z(c) {};
	};

	struct RG_factor
	{
		_IN		Seed_s	seed;			// �ؤl�I
		_IN		int		s_kernel;		// �ؤl�I�F��d��
		_IN		int		n_kernel;		// �ؤl�I�F�񹳯����P��d��
		_IN		double	pix_thresh;		// �������� : ���� �H��
		_IN		double	sd_thresh;		// �������� : �зǮt �H��
		_IN		double	sd_coeffi;		// �������� : �зǮt ���v
	};

	Seed_d			_seed_gl;			// �b3D�����I����seed openGL�@�ɮy��
	Seed_s			_seed_pt;			// �b2D�����I����seed�y��
	Seed_s			_seed_img;			// 3D_seed���@�ɮy���ഫ����v��"�x�}"(512*512)�y��

	bool			_get_3D_seed;		// �O�_�I��F3D�ؤl�I
	bool			_get_2D_seed;		// �O�_�I��F2D�ؤl�I
	bool			_get_region_grow;	// �O�_����L�ϰ즨��

	float			_x_index;			// �ե� X �b��Ҫ��Ѽ�
	float			_y_index;			// �ե� Y �b��Ҫ��Ѽ�
	float			_z_index;			// �ե� Z �b��Ҫ��Ѽ�

	short**			_judge;				// �O���ϰ즨�����G(�����P�w)
	BYTE			_image0[256 * 256 * 256][4];

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///



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
	void	Draw2DImage(const int& slice);				// ø�s�G���v��
	void*	new2Dmatrix(int h, int w, int size);		// �ʺA�t�m�G���x�}
	
	void	InvertMat(float(&m)[16]);
	void	ActTracking(int x, int y);					// ����ާ@���l��(�]�t����P�첾)
	void	ActStop(UINT nFlags, int x, int y);			// �u��������v���ʧ@�]�w
	void	ActStart(UINT nFlags, int x, int y);		// �u�}�l����v���ʧ@�]�w
	void	pointToVector(int x, int y, int width, int height, float vec[3]);

	Seed_s	coordiConvert(Seed_d& pt);					// openGL coordinate -> data array site

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
	
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();

	afx_msg void OnBnClickedButtonPhantomOpen();

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
