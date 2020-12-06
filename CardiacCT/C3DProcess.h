
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

	short			HU_down_threshold;		// 二值化 : 低閾值 (HU)
	short			HU_up_threshold;		// 二值化 : 高閾值 (HU)
	unsigned short	pixel_down_threshold;	// 二值化 : 低閾值 (pixel)
	unsigned short	pixel_up_threshold;		// 二值化 : 高閾值 (pixel)
	unsigned short	DisplaySlice;			// 顯示的slice(從0開始)

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

	Seed_d			seed_gl;			// 在3D視窗點擊的seed openGL世界座標
	Seed_s			seed_pt;			// 在2D視窗點擊的seed座標
	Seed_s			seed_img;			// 3D_seed的世界座標轉換為原影像"矩陣"(512*512)座標

	bool			get_3Dseed;			// 是否點選了3D種子點
	bool			get_2Dseed;			// 是否點選了2D種子點
	bool			get_regionGrow;		// 是否執行過區域成長

	float			x_index;			// 校正 X 軸比例的參數
	float			y_index;			// 校正 Y 軸比例的參數
	float			z_index;			// 校正 Z 軸比例的參數

	short**			judge;				// 記錄區域成長結果(成長判定)
	BYTE			m_image0[256*256*256][4];

///-------------------------↑ 3D seed 宣告參數 ↑---------------------------------------///

///------------- ↓ 3D 實驗區 ↓ -------------///

	RG_factor		RG_term;			// 3D區域成長 : 條件因子
	double			spine_volume;		// 3D區域成長 : 脊椎骨體積
	double			sternum_volume;		// 3D區域成長 : 胸骨體積

	bool			get_spine;
	bool			get_sternum;
	bool			get_mid_fix;
	pair<int, int>	mid_fix_pt;

	//std::vector<int> judge_type;										// 紀錄判定類型 (+ : 要的 , - : 不要的)
																		//  0 : 還沒判斷
																		//  1.2 : spine (1RG, 2RG)
																		//  3.4 : sternum (1RG, 2RG) 

	std::map<int, std::vector<std::pair<int, int>>> spine_vertex;		// 紀錄脊椎初步處理後的三個頂點
																		// 0 : 中間上面那點
																		// 1 : 左下
																		// 2 : 右下

	std::map<int, std::vector<std::pair<float, float>>> spine_line;		// 紀錄脊椎每張slice的直線方程式的係數(斜率.截距)
																		// 0 : 左
																		// 1 : 右

	std::map<int, std::vector<int>> spine_edge;							// 記錄脊椎每張slice將進行處理的範圍(垂直邊界)
																		// 0 : x_min
																		// 1 : x_max
																		// 2 : y_min
																		// 3 : y_max

	std::map<int, std::vector<std::pair<int, int>>> sternum_vertex;		// 紀錄胸骨每張slice的三頂點
																		// 0 : 中間上面那點
																		// 1 : 左下
																		// 2 : 右下

	std::map<int, std::vector<std::pair<float, float>>> sternum_line;	// 紀錄胸骨每張slice的直線方程式係數(斜率.截距)
																		// 0 : 左
																		// 1 : 右

	std::map<int, std::vector<int>> sternum_edge;						// 記錄胸骨每張slice將進行處理的範圍(垂直邊界)
																		// 0 : x_min
																		// 1 : x_max
																		// 2 : y_min
																		// 3 : y_max

///------------- ↑ 3D 實驗區 ↑ -------------///

///------------- ↓ 2D 實驗區(verify的東西) ↓ -------------///

	int		draw_pt_cnt;					// 畫線時，畫到第幾點
	int		draw_pt_total;					// 畫線時，共幾點畫線

	int		verify_reference_slice;
	bool	get_verify_reference;


	std::vector<int> spine_interpolate_slice;							// 紀錄用來內插驗證線的切片
	std::vector<int> sternum_interpolate_slice;							// 紀錄用來內插驗證線的切片

	std::map<int, std::vector<std::pair<int, int>>> draw_spine_pt;		// 畫線時，點擊繪製的點
	std::map<int, std::vector<std::pair<int, int>>> draw_sternum_pt;	// 畫線時，點擊繪製的點

	std::map<int, std::set<std::pair<int, int>>> draw_spine_line;		// 畫線時，點的那些點所連成的線
	std::map<int, std::set<std::pair<int, int>>> draw_sternum_line;		// 畫線時，點的那些點所連成的線
	

///------------- ↑ 2D 實驗區(verify的東西) ↑ -------------///

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

	Seed_s	coordiConvert(Seed_d& pt);					// openGL coordinate -> data array site

///------------- ↓ 實驗區 ↓ -------------///

	double	Calculate_Volume(short** src);
	void	Erosion_3D(short** src, short element);
	void	Dilation_3D(short** src, short element);

	void	Spine_process();											// 脊椎二次成長 - 預處理
	void	Spine_process_fix();										// 脊椎二次成長 - 修正成長後的資訊
	void	RG2_3D_Spine_process(short** src, RG_factor& factor);		// 脊椎二次成長 - 限制線與全域平均
	void	RG_3D_Spine_process(short** src, RG_factor& facotr);		// 脊椎一次成長 - 限制線與標準差

	void	Sternum_process();											// 胸骨二次成長 - 預處理
	void	Sternum_process_fix();										// 胸骨二次成長 - 修正成長後的資訊
	void	RG2_3D_Sternum_process(short** src, RG_factor& factor);		// 胸骨二次成長 - 限制線與全域平均
	void	RG_3D_Sternum_process(short** src, RG_factor& factor);		// 胸骨一次成長 - 限制線與標準差
	
	void	RG_3D_ProposedMethod(short**src, RG_factor& factor);		// 3D 區域成長(當前區域標準差與全域平均)
	void	RG_3D_GlobalAvgConnected(short** src, RG_factor& factor);	// 3D 區域成長(全域平均)
	void	RG_3D_ConfidenceConnected(short** src, RG_factor& factor);	// 3D 區域成長(當前區域標準差與平均) 
	

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
