
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
	CCTDoc*			m_pDoc;				// 紀錄Doc的物件指標
	
	CScrollBar		m_ScrollBar;

	HDC				m_hDC;				// MFC視窗DC
	HGLRC			m_hRC;				// openGL視窗DC
	CDIB*			m_2D_dib;
	CRect			m_3D_rect;
	CRect			m_2D_rect;
	CWnd*			m_3D_frame;
	CWnd*			m_2D_frame;
	BOOL			gl_3DTexture;		// 判斷是否支援3D紋理
PFNGLTEXIMAGE3DPROC glTexImage3D;		// Address of an openGL extension function.
	
	GLuint			textureName[5];		// 記載紋理名稱
	
///-------------------------↓ openGL 設定參數 ↓----------------------------------------///
	int				Mat_Offset;			// 3D影像矩陣置中的偏移量
	int				ImageFrame;			// 影像框數(1個時序1個)
	int				glSlices;			// openGL建立紋理層數

	bool			LR_Button;			// true:滑鼠左鍵；false:滑鼠右鍵
	bool			Act_Rotate;
	bool			Act_Translate;
	bool			get_3Dimage;		// 是否已經建立 3D image

	float			scale_x;			// 可以改顯示比例的神奇參數
	float			scale_y;			// 此三項數值為反覆測試後所設定的
	float			scale_z;
	float			density;
	float			intensity;

	float*			obj_axis;			// 控制 物件 旋轉軸
	float*			pln_axis;			// 控制 平面 旋轉軸
	float			obj_angle;			// 控制 物件 旋轉角度(Rotation)
	float			pln_angle;			// 控制 平面 旋轉角度(Rotation)
	float			viewDistance;		// 控制 視線 位移(Translation)
	
	float			transY;
	float*			lastPos;
	float**			glVertexPt;			// openGL繪圖點
	double*			user_Plane;
///-------------------------↑ openGL 設定參數 ↑----------------------------------------///

///-------------------------↓ 3D seed 宣告參數 ↓---------------------------------------///

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
		_IN		Seed_s	seed;			// 種子點
		_IN		int		kernel;			// 拜託務必保持奇數
		_IN		int		z_upLimit;		// Z軸成長上限(最多到 0)
		_IN		int		z_downLimit;	// Z軸成長下限(最多到 TotalSlice)
		_IN		double	threshold;		// 成長閾值
		_OUT	double	growingVolume;	// 成長體積
	}	RG_Factor;

	RG_Factor		RG_Total;			// 區域成長條件與情況
	Seed_d			seed_gl;			// 在3D視窗點擊的seed openGL世界座標
	Seed_s			seed_pt;			// 在2D視窗點擊的seed座標
	Seed_s			seed_img;			// 3D_seed的世界座標轉換為原影像"矩陣"(512*512)座標

	bool			get_3Dseed;			// 是否點選了3D種子點
	bool			get_2Dseed;			// 是否點選了2D種子點
	bool			get_regionGrow;		// 是否執行過區域成長

	float			x_index;			// 校正 X 軸比例的參數
	float			y_index;			// 校正 Y 軸比例的參數
	float			z_index;			// 校正 Z 軸比例的參數

	BYTE**			judge;				// 區域成長判定
	BYTE			m_image0[256*256*256][4];

///-------------------------↑ 3D seed 宣告參數 ↑---------------------------------------///

	short			HUThreshold;		// 二值化閾值(HU)
unsigned short		PixelThreshold;		// 二值化閾值(pixel)
unsigned short		DisplaySlice;		// 顯示的slice(從0開始)

///------------- ↓ 實驗區 ↓ -------------///



///------------- ↑ 實驗區 ↑ -------------///

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
	void	Draw2DImage(unsigned short& slice);			// 繪製二維影像
	void*	new2Dmatrix(int h, int w, int size);		// 動態配置二維矩陣
	void*	new3Dmatrix(int h, int w, int l, int size);
	void*	new4Dmatrix(int h, int w, int l, int v, int size);

	void	InvertMat(float (&m)[16]);
	void	ActTracking(int x, int y);					// 物件操作的追蹤(包含旋轉與位移)
	void	ActStop(UINT nFlags, int x, int y);			// 「結束旋轉」的動作設定
	void	ActStart(UINT nFlags, int x, int y);		// 「開始旋轉」的動作設定
	void	pointToVector(int x, int y, int width, int height, float vec[3]);

	void	Region_Growing_3D(RG_Factor& factor);		// 3D 區域成長(成長結果存於judge)
	Seed_s	coordiConvert(Seed_d& pt);					// openGL coordinate -> data array site

///------------- ↓ 實驗區 ↓ -------------///

	void	Erosion_3D(); 

///------------- ↑ 實驗區 ↑ -------------///
	
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
public:										// CString的部分，有在Attributes另外設變數儲存
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
