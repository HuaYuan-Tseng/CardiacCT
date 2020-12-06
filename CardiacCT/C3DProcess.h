
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

#include <vector>
#include <map>
#include <set>

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

	short			HU_down_threshold;		// �G�Ȥ� : �C�H�� (HU)
	short			HU_up_threshold;		// �G�Ȥ� : ���H�� (HU)
	unsigned short	pixel_down_threshold;	// �G�Ȥ� : �C�H�� (pixel)
	unsigned short	pixel_up_threshold;		// �G�Ȥ� : ���H�� (pixel)
	unsigned short	DisplaySlice;			// ��ܪ�slice(�q0�}�l)

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

	Seed_d			seed_gl;			// �b3D�����I����seed openGL�@�ɮy��
	Seed_s			seed_pt;			// �b2D�����I����seed�y��
	Seed_s			seed_img;			// 3D_seed���@�ɮy���ഫ����v��"�x�}"(512*512)�y��

	bool			get_3Dseed;			// �O�_�I��F3D�ؤl�I
	bool			get_2Dseed;			// �O�_�I��F2D�ؤl�I
	bool			get_regionGrow;		// �O�_����L�ϰ즨��

	float			x_index;			// �ե� X �b��Ҫ��Ѽ�
	float			y_index;			// �ե� Y �b��Ҫ��Ѽ�
	float			z_index;			// �ե� Z �b��Ҫ��Ѽ�

	short**			judge;				// �O���ϰ즨�����G(�����P�w)
	BYTE			m_image0[256*256*256][4];

///-------------------------�� 3D seed �ŧi�Ѽ� ��---------------------------------------///

///------------- �� 3D ����� �� -------------///

	RG_factor		RG_term;			// 3D�ϰ즨�� : ����]�l
	double			spine_volume;		// 3D�ϰ즨�� : ��հ���n
	double			sternum_volume;		// 3D�ϰ즨�� : �ݰ���n

	bool			get_spine;
	bool			get_sternum;
	bool			get_mid_fix;
	pair<int, int>	mid_fix_pt;

	//std::vector<int> judge_type;										// �����P�w���� (+ : �n�� , - : ���n��)
																		//  0 : �٨S�P�_
																		//  1.2 : spine (1RG, 2RG)
																		//  3.4 : sternum (1RG, 2RG) 

	std::map<int, std::vector<std::pair<int, int>>> spine_vertex;		// ������ժ�B�B�z�᪺�T�ӳ��I
																		// 0 : �����W�����I
																		// 1 : ���U
																		// 2 : �k�U

	std::map<int, std::vector<std::pair<float, float>>> spine_line;		// ������ըC�islice�����u��{�����Y��(�ײv.�I�Z)
																		// 0 : ��
																		// 1 : �k

	std::map<int, std::vector<int>> spine_edge;							// �O����ըC�islice�N�i��B�z���d��(�������)
																		// 0 : x_min
																		// 1 : x_max
																		// 2 : y_min
																		// 3 : y_max

	std::map<int, std::vector<std::pair<int, int>>> sternum_vertex;		// �����ݰ��C�islice���T���I
																		// 0 : �����W�����I
																		// 1 : ���U
																		// 2 : �k�U

	std::map<int, std::vector<std::pair<float, float>>> sternum_line;	// �����ݰ��C�islice�����u��{���Y��(�ײv.�I�Z)
																		// 0 : ��
																		// 1 : �k

	std::map<int, std::vector<int>> sternum_edge;						// �O���ݰ��C�islice�N�i��B�z���d��(�������)
																		// 0 : x_min
																		// 1 : x_max
																		// 2 : y_min
																		// 3 : y_max

///------------- �� 3D ����� �� -------------///

///------------- �� 2D �����(verify���F��) �� -------------///

	int		draw_pt_cnt;					// �e�u�ɡA�e��ĴX�I
	int		draw_pt_total;					// �e�u�ɡA�@�X�I�e�u

	int		verify_reference_slice;
	bool	get_verify_reference;


	std::vector<int> spine_interpolate_slice;							// �����ΨӤ������ҽu������
	std::vector<int> sternum_interpolate_slice;							// �����ΨӤ������ҽu������

	std::map<int, std::vector<std::pair<int, int>>> draw_spine_pt;		// �e�u�ɡA�I��ø�s���I
	std::map<int, std::vector<std::pair<int, int>>> draw_sternum_pt;	// �e�u�ɡA�I��ø�s���I

	std::map<int, std::set<std::pair<int, int>>> draw_spine_line;		// �e�u�ɡA�I�������I�ҳs�����u
	std::map<int, std::set<std::pair<int, int>>> draw_sternum_line;		// �e�u�ɡA�I�������I�ҳs�����u
	

///------------- �� 2D �����(verify���F��) �� -------------///

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

	Seed_s	coordiConvert(Seed_d& pt);					// openGL coordinate -> data array site

///------------- �� ����� �� -------------///

	double	Calculate_Volume(short** src);
	void	Erosion_3D(short** src, short element);
	void	Dilation_3D(short** src, short element);

	void	Spine_process();											// ��դG������ - �w�B�z
	void	Spine_process_fix();										// ��դG������ - �ץ������᪺��T
	void	RG2_3D_Spine_process(short** src, RG_factor& factor);		// ��դG������ - ����u�P���쥭��
	void	RG_3D_Spine_process(short** src, RG_factor& facotr);		// ��դ@������ - ����u�P�зǮt

	void	Sternum_process();											// �ݰ��G������ - �w�B�z
	void	Sternum_process_fix();										// �ݰ��G������ - �ץ������᪺��T
	void	RG2_3D_Sternum_process(short** src, RG_factor& factor);		// �ݰ��G������ - ����u�P���쥭��
	void	RG_3D_Sternum_process(short** src, RG_factor& factor);		// �ݰ��@������ - ����u�P�зǮt
	
	void	RG_3D_ProposedMethod(short**src, RG_factor& factor);		// 3D �ϰ즨��(��e�ϰ�зǮt�P���쥭��)
	void	RG_3D_GlobalAvgConnected(short** src, RG_factor& factor);	// 3D �ϰ즨��(���쥭��)
	void	RG_3D_ConfidenceConnected(short** src, RG_factor& factor);	// 3D �ϰ즨��(��e�ϰ�зǮt�P����) 
	

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
	BOOL		m_spine;
	BOOL		m_sternum;
	BOOL		m_spine_verify;
	BOOL		m_sternum_verify;
	BOOL		m_plane;
	BOOL		m_object;
	BOOL		m_2Dseed;
	BOOL		m_3Dseed;
	BOOL		m_2Dverify;
	BOOL		m_disp_org;
	BOOL		m_disp_pro0;
	BOOL		m_complete;
	BOOL		m_thresholdHU;
	BOOL		m_thresholdPixel;

	CString		m_pixel_down_threshold;
	CString		m_pixel_up_threshold;
	CString		m_HU_down_threshold;
	CString		m_HU_up_threshold;
	CString		m_intensity;
	CString		m_density;
	CString		m_slices;
	CString		m_pos_1;
	CString		m_pos_2;
	CString		m_pos_3;
	CString		m_pos_4;
	CString		m_pos_5;
	CString		m_pos_6;
	CString		m_pos_7;
	CString		m_pos_8;
	CString		m_result;
	CString		m_result_2;
	CString		m_sKernel;
	CString		m_nKernel;
	CString		m_pix_th;
	CString		m_SDth;
	CString		m_SDco;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnBnClickedExit();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
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
	afx_msg void OnBnClickedCheckDispPro0();
	afx_msg void OnBnClickedCheckDispOrg();
	afx_msg void OnBnClickedCheckSternum();
	afx_msg void OnBnClickedCheckSpine();
	afx_msg void OnBnClickedCheck2dSeed();
	afx_msg void OnBnClickedCheck2dVerify();
	afx_msg void OnBnClickedCheckSpineVerify();
	afx_msg void OnBnClickedCheckSternumVerify();

	afx_msg void OnEnChangeEditSlices();
	afx_msg void OnEnChangeEditHuUpThreshold();
	afx_msg void OnEnChangeEditHuDownThreshold();
	afx_msg void OnEnChangeEditPixelUpThreshold();
	afx_msg void OnEnChangeEditPixelDownThreshold();
	afx_msg void OnEnChangeEditSKernel();
	afx_msg void OnEnChangeEditNKernel();
	afx_msg void OnEnChangeEditPixTh();
	afx_msg void OnEnChangeEditSdTh();
	afx_msg void OnEnChangeEditSdCo();

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
	afx_msg void OnBnClickedButtonGrowingRecovery();
	afx_msg void OnBnClickedButtonGrowingCapture();
	afx_msg void OnBnClickedButtonGrowingRemove();
	afx_msg void OnBnClickedButtonGrowingClear();
	afx_msg void OnBnClickedButtonRecordLimit();
	afx_msg void OnBnClickedButtonReuseLimit();
	afx_msg void OnBnClickedButtonDilation();
	afx_msg void OnBnClickedButtonMidFix();
	afx_msg void OnBnClickedButtonVerifySave();
	afx_msg void OnBnClickedButtonVerifyLoad();
	afx_msg void OnBnClickedButtonVerifyReuse();
	afx_msg void OnBnClickedButtonVerifyRecord();
	afx_msg void OnBnClickedButtonVerifyCalculate();
	afx_msg void OnBnClickedButtonVerifyLineErase();
	afx_msg void OnBnClickedButtonVerifyLineClear();
	afx_msg void OnBnClickedButtonVerifyInterpolation();
	afx_msg void OnBnClickedButtonVerifyLineReference();
	afx_msg void OnBnClickedButtonVerifyLineCancelReference();
	
};
