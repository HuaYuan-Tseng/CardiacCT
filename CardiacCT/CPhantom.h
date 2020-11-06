
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
	
	enum class ControlModes				// 物件操作模式
	{
		ControlNone,					// 不執行任何操作
		ControlObject,					//  -心臟- 操作
		ControlPlane,					//  -平面- 操作
	}	mode;

//================//
//   Attributes   //
//================//
public:

	CScrollBar		_scroll_bar;

	HDC				_hDC;				// MFC視窗DC
	HGLRC			_hRC;				// openGL視窗DC
	CDIB*			_2D_dib;
	CRect			_2D_rect;
	CRect			_3D_rect;
	CWnd*			_2D_frame;
	CWnd*			_3D_frame;

	BOOL			gl_3DTexture;		// 判斷是否支援3D紋理
	PFNGLTEXIMAGE3DPROC glTexImage3D;	// Address of an openGL extension function.

	GLuint			_textureName[5];	// 記載紋理名稱

///-------------------------↓ 2D影像 設定參數 ↓----------------------------------------///

	BYTE**			_img;				// 原始灰階資訊

	bool			_get_2D_image;
	int				_display_slice;		// 顯示的切片
	int				_total_slice;		// 總張數
	int				_row;				// 影像(列)
	int				_col;				// 影像(行)

///-------------------------↑ 2D影像 設定參數 ↑----------------------------------------///

///-------------------------↓ openGL 設定參數 ↓----------------------------------------///
	int				_mat_offset;		// 3D影像矩陣置中的偏移量
	int				_image_frame;		// 影像框數(1個時序1個)
	int				_gl_slices;			// openGL建立紋理層數

	bool			_LR_button;			// true:滑鼠左鍵；false:滑鼠右鍵
	bool			_act_rotate;
	bool			_act_translate;
	bool			_get_3D_image;		// 是否已經建立 3D image
	bool			_get_GL_build;		// 是否已建立openGL的環境

	float			_scale_x;			// 可以改顯示比例的神奇參數
	float			_scale_y;			// 此三項數值為反覆測試後所設定的
	float			_scale_z;
	float			_density;
	float			_intensity;

	float*			_obj_axis;			// 控制 物件 旋轉軸
	float*			_pln_axis;			// 控制 平面 旋轉軸
	float			_obj_angle;			// 控制 物件 旋轉角度(Rotation)
	float			_pln_angle;			// 控制 平面 旋轉角度(Rotation)
	float			_view_distance;		// 控制 視線 位移(Translation)

	float			_transY;
	float*			_last_pos;
	float**			_glVertexPt;		// openGL繪圖點
	double*			_user_plane;
///-------------------------↑ openGL 設定參數 ↑----------------------------------------///

///-------------------------↓ 3D seed 宣告參數 ↓---------------------------------------///

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
		_IN		Seed_s	seed;			// 種子點
		_IN		int		s_kernel;		// 種子點鄰近範圍
		_IN		int		n_kernel;		// 種子點鄰近像素的周邊範圍
		_IN		double	pix_thresh;		// 成長條件 : 像素 閾值
		_IN		double	sd_thresh;		// 成長條件 : 標準差 閾值
		_IN		double	sd_coeffi;		// 成長條件 : 標準差 倍率
	};

	Seed_d			_seed_gl;			// 在3D視窗點擊的seed openGL世界座標
	Seed_s			_seed_pt;			// 在2D視窗點擊的seed座標
	Seed_s			_seed_img;			// 3D_seed的世界座標轉換為原影像"矩陣"(512*512)座標

	bool			_get_3D_seed;		// 是否點選了3D種子點
	bool			_get_2D_seed;		// 是否點選了2D種子點
	bool			_get_region_grow;	// 是否執行過區域成長

	float			_x_index;			// 校正 X 軸比例的參數
	float			_y_index;			// 校正 Y 軸比例的參數
	float			_z_index;			// 校正 Z 軸比例的參數

	short**			_judge;				// 記錄區域成長結果(成長判定)
	BYTE			_image0[256 * 256 * 256][4];

///-------------------------↑ 3D seed 宣告參數 ↑---------------------------------------///



//================//
//   Operations   //
//================//
public:
	BOOL	SetupPixelFormat(HDC hDC);					// 設置hDC像素格式
	BOOL	ExtensionSupported(const char* exten);

	void	LoadVolume();								// 建立紋理
	void	PrepareVolume();							// 建立紋理座標的資料矩陣
	void	GLInitialization();							// openGL建構初始化
	void	PerspectiveBuild();							// 建立透視空間
	void	getRamp(GLubyte* color, float t, int n);	// 上色

	void	Draw3DImage(bool which);					// 繪製三維影像
	void	Draw2DImage(const int& slice);				// 繪製二維影像
	void*	new2Dmatrix(int h, int w, int size);		// 動態配置二維矩陣
	
	void	InvertMat(float(&m)[16]);
	void	ActTracking(int x, int y);					// 物件操作的追蹤(包含旋轉與位移)
	void	ActStop(UINT nFlags, int x, int y);			// 「結束旋轉」的動作設定
	void	ActStart(UINT nFlags, int x, int y);		// 「開始旋轉」的動作設定
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
