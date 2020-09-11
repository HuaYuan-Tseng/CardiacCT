
// C3DProcess.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "C3DProcess.h"
#include "CProgress.h"
#include "CWait.h"
#include "afxdialogex.h"
#include <ctime>
#include <cmath>
#include <thread>
#include <numeric>
#include <algorithm>

#include <map>
#include <queue>
#include <vector>
#include <unordered_map>

using namespace std;
constexpr auto M_PI = 3.1415926F;

#define ROW m_pDoc->m_dir->Row
#define COL m_pDoc->m_dir->Col
#define HU_Min m_pDoc->m_dir->HU_min
#define HU_Max m_pDoc->m_dir->HU_max
#define Display_Series m_pDoc->displaySeries
#define VoxelSpacing_X m_pDoc->m_dir->Voxel_Spacing_X
#define VoxelSpacing_Y m_pDoc->m_dir->Voxel_Spacing_Y
#define VoxelSpacing_Z m_pDoc->m_dir->Voxel_Spacing_Z
#define RescaleSlope atoi(m_pDoc->m_dir->Rescale_Slope)
#define RescaleIntercept atoi(m_pDoc->m_dir->Rescale_Intercept)
#define Total_Slice m_pDoc->m_dir->SeriesList[0]->TotalSliceCount

#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))
#define New3Dmatrix(H, W, L, TYPE) (TYPE***)new3Dmatrix(H, W, L, sizeof(TYPE))
#define New4Dmatrix(H, W, L, V, TYPE) (TYPE****)new4Dmatrix(H, W, L, V, sizeof(TYPE))

//==========================//
//   3D Processing Dialog   //
//==========================//

IMPLEMENT_DYNAMIC(C3DProcess, CDialogEx)

C3DProcess::C3DProcess(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPROCESS, pParent)
	, m_3Dseed(FALSE)
	, m_object(TRUE)
	, m_plane(FALSE)
	, m_disp_org(TRUE)
	, m_disp_pro0(FALSE)
	, m_complete(TRUE)
	, m_thresholdHU(FALSE)
	, m_thresholdPixel(FALSE)
	, m_HUThreshold(_T("210"))
	, m_pixelThreshold(_T("150"))
	, m_intensity(_T("0.8125"))
	, m_density(_T("0.0"))
	, m_slices(_T("512"))
	, m_result(_T("0.0"))
	, m_pos_1(_T("0"))
	, m_pos_2(_T("0"))
	, m_pos_3(_T("0"))
	, m_pos_4(_T("0"))
	, m_pos_5(_T("0"))
	, m_pos_6(_T("0"))
	, m_pos_7(_T("0"))
	, m_pos_8(_T("0"))
{
	mode = ControlModes::ControlObject;

	m_pDoc = nullptr;
	m_2D_dib = nullptr;
	m_2D_frame = nullptr;
	m_3D_frame = nullptr;
	gl_3DTexture = FALSE;

	Act_Translate = false;
	Act_Rotate = false;
	get_2Dseed = false;
	get_3Dseed = false;
	get_regionGrow = false;
	get_3Dimage = false;

	x_index = 0.5F;
	y_index = 0.5F;
	z_index = 0.7F;
	scale_x = 0.3F;
	scale_y = 0.5F;
	scale_z = 0.5F;
	density = 0.000F;
	intensity = 0.8125F;
	viewDistance = -4.0F;

	transY = 0.0F;
	ImageFrame = 1;
	glSlices = 512;
	obj_angle = 0.0F;
	pln_angle = 0.0F;
	DisplaySlice = 0;
	RG_totalVolume = 0.0F;
	HUThreshold = atoi(m_HUThreshold);
	PixelThreshold = atoi(m_pixelThreshold);

	seed_pt = { 0, 0, 0 };
	seed_img = { 0, 0, 0 };
	seed_gl = { 0.0L, 0.0L, 0.0L };
	
	glVertexPt = New2Dmatrix(64, 3, float);
	lastPos = new float[3]{ 0.0F, 0.0F, 0.0F };
	obj_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	pln_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	user_Plane = new double[4]{ 1.0L, 0.0L, 0.0L, 1.0L };
}

C3DProcess::~C3DProcess()
{
	if (m_2D_dib != nullptr)
		delete  m_2D_dib;
	if (user_Plane != nullptr)
		delete[] user_Plane;
	if (glVertexPt != nullptr)
		delete[] glVertexPt;
	if (obj_axis != nullptr)
		delete[] obj_axis;
	if (pln_axis != nullptr)
		delete[] pln_axis;
	if (lastPos != nullptr)
		delete[] lastPos;
	if (judge != nullptr)
		delete[] judge;
	
	if (m_pixelThreshold.IsEmpty() != true)
		m_pixelThreshold.Empty();
	if (m_HUThreshold.IsEmpty() != true)
		m_HUThreshold.Empty();
	if (m_intensity.IsEmpty() != true)
		m_intensity.Empty();
	if (m_density.IsEmpty() != true)
		m_density.Empty();
	if (m_slices.IsEmpty() != true)
		m_slices.Empty();
	if (m_result.IsEmpty() != true)
		m_result.Empty();
	if (m_pos_1.IsEmpty() != true)
		m_pos_1.Empty();
	if (m_pos_2.IsEmpty() != true)
		m_pos_2.Empty();
	if (m_pos_3.IsEmpty() != true)
		m_pos_3.Empty();
	if (m_pos_4.IsEmpty() != true)
		m_pos_4.Empty();
	if (m_pos_5.IsEmpty() != true)
		m_pos_5.Empty();
	if (m_pos_6.IsEmpty() != true)
		m_pos_6.Empty();
	if (m_pos_7.IsEmpty() != true)
		m_pos_7.Empty();
	if (m_pos_8.IsEmpty() != true)
		m_pos_8.Empty();

	glDeleteTextures(ImageFrame, textureName);
}

void C3DProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_2D, m_ScrollBar);

	DDX_Check(pDX, IDC_CHECK_PLANE, m_plane);
	DDX_Check(pDX, IDC_CHECK_Object, m_object);
	DDX_Check(pDX, IDC_CHECK_3D_SEED, m_3Dseed);
	DDX_Check(pDX, IDC_CHECK_DISP_ORG, m_disp_org);
	DDX_Check(pDX, IDC_CHECK_DISP_PRO0, m_disp_pro0);
	DDX_Check(pDX, IDC_CHECK_COMPLETE, m_complete);
	DDX_Check(pDX, IDC_CHECK_HU_THRESHOLD, m_thresholdHU);
	DDX_Check(pDX, IDC_CHECK_PIXEL_THRESHOLD, m_thresholdPixel);

	DDX_Text(pDX, IDC_EDIT_SLICES, m_slices);
	DDV_MinMaxInt(pDX, atoi(m_slices), 1, 512);

	DDX_Text(pDX, IDC_EDIT_HU_THRESHOLD, m_HUThreshold);
	DDV_MinMaxShort(pDX, atoi(m_HUThreshold), HU_Min, HU_Max);

	DDX_Text(pDX, IDC_EDIT_PIXEL_THRESHOLD, m_pixelThreshold);
	DDV_MinMaxShort(pDX, atoi(m_pixelThreshold), 0, 255);

	DDX_Text(pDX, IDC_EDIT_INTENSITY, m_intensity);
	DDX_Text(pDX, IDC_EDIT_DENSITY, m_density);
	DDX_Text(pDX, IDC_EDIT_RESULT, m_result);
	DDX_Text(pDX, IDC_EDIT_POS1, m_pos_1);
	DDX_Text(pDX, IDC_EDIT_POS2, m_pos_2);
	DDX_Text(pDX, IDC_EDIT_POS3, m_pos_3);
	DDX_Text(pDX, IDC_EDIT_POS4, m_pos_4);
	DDX_Text(pDX, IDC_EDIT_POS5, m_pos_5);
	DDX_Text(pDX, IDC_EDIT_POS6, m_pos_6);
	DDX_Text(pDX, IDC_EDIT_POS7, m_pos_7);
	DDX_Text(pDX, IDC_EDIT_POS8, m_pos_8);
}

BEGIN_MESSAGE_MAP(C3DProcess, CDialogEx)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()

	ON_BN_CLICKED(IDC_CHECK_3D_SEED, &C3DProcess::OnBnClickedCheck3dSeed)
	ON_BN_CLICKED(IDC_CHECK_PLANE, &C3DProcess::OnBnClickedCheckPlane)
	ON_BN_CLICKED(IDC_CHECK_Object, &C3DProcess::OnBnClickedCheckObject)
	ON_BN_CLICKED(IDC_CHECK_DISP_ORG, &C3DProcess::OnBnClickedCheckDispOrg)
	ON_BN_CLICKED(IDC_CHECK_DISP_PRO0, &C3DProcess::OnBnClickedCheckDispPro0)
	ON_BN_CLICKED(IDC_CHECK_COMPLETE, &C3DProcess::OnBnClickedCheckComplete)
	ON_BN_CLICKED(IDC_CHECK_HU_THRESHOLD, &C3DProcess::OnBnClickedCheckHuThreshold)
	ON_BN_CLICKED(IDC_CHECK_PIXEL_THRESHOLD, &C3DProcess::OnBnClickedCheckPixelThreshold)
	
	ON_BN_CLICKED(IDC_BUTTON_PLANE_RESET, &C3DProcess::OnBnClickedButtonPlaneReset)
	ON_BN_CLICKED(IDC_BUTTON_SLICES_PLUS, &C3DProcess::OnBnClickedButtonSlicesPlus)
	ON_BN_CLICKED(IDC_BUTTON_SLICES_MINUS, &C3DProcess::OnBnClickedButtonSlicesMinus)
	ON_BN_CLICKED(IDC_BUTTON_DENSITY_PLUS, &C3DProcess::OnBnClickedButtonDensityPlus)
	ON_BN_CLICKED(IDC_BUTTON_DENSITY_MINUS, &C3DProcess::OnBnClickedButtonDensityMinus)
	ON_BN_CLICKED(IDC_BUTTON_INTENSITY_PLUS, &C3DProcess::OnBnClickedButtonIntensityPlus)
	ON_BN_CLICKED(IDC_BUTTON_INTENSITY_MINUS, &C3DProcess::OnBnClickedButtonIntensityMinus)
	ON_BN_CLICKED(IDC_BUTTON_REGION_GROWING, &C3DProcess::OnBnClickedButtonRegionGrowing)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_RECOVERY, &C3DProcess::OnBnClickedButtonGrowingRecovery)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_REMOVE, &C3DProcess::OnBnClickedButtonGrowingRemove)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_CLEAR, &C3DProcess::OnBnClickedButtonGrowingClear)
	ON_BN_CLICKED(IDC_BUTTON_2DSEED_CLEAR, &C3DProcess::OnBnClickedButton2dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_3DSEED_CLEAR, &C3DProcess::OnBnClickedButton3dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_SEED_CHANGE, &C3DProcess::OnBnClickedButtonSeedChange)
	ON_BN_CLICKED(IDC_BUTTON_DILATION, &C3DProcess::OnBnClickedButtonDilation)

	ON_EN_CHANGE(IDC_EDIT_SLICES, &C3DProcess::OnEnChangeEditSlices)
	ON_EN_CHANGE(IDC_EDIT_HU_THRESHOLD, &C3DProcess::OnEnChangeEditHuThreshold)
	ON_EN_CHANGE(IDC_EDIT_PIXEL_THRESHOLD, &C3DProcess::OnEnChangeEditPixelThreshold)
END_MESSAGE_MAP()

//=================================//
//   C3DProcess message handlers   //
//=================================//

BOOL C3DProcess::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	// Dialog 初始化的事件
	//

	// 更換Dialog標題
	//
	SetWindowText("3D Processing");

	// 設定繪圖座標軸、區域與物件
	//
	m_2D_frame = GetDlgItem(IDC_STATIC_2D);									// 訪問 Dialog 中的物件。
	m_2D_frame->GetWindowRect(&m_2D_rect);									// 獲得物件在螢幕上的位置（以螢幕為座標軸）。
	ScreenToClient(&m_2D_rect);												// 將物件在以"螢幕"為座標軸體系下的位置，轉換為"用戶區"座標軸體系下的位置。
	//m_2D_frame->MoveWindow(m_2D_rect.left, m_2D_rect.top, COL, ROW, true);// 調整物件位置與大小。

	m_3D_frame = GetDlgItem(IDC_STATIC_3D);
	m_3D_frame->GetWindowRect(&m_3D_rect);
	ScreenToClient(&m_3D_rect);
	//m_3D_frame->MoveWindow(m_3D_rect.left, m_3D_rect.top, COL, ROW, true);

	m_2D_dib = new CDIB();							
	m_2D_dib->InitDIB(COL, ROW);											// 初始化畫框

	// 設定ScrollBar的範圍
	//
	m_ScrollBar.SetScrollRange(0, Total_Slice-1);

	//-------------------------------------------------------------------------------------//
	// 設定Button初始狀態
	//
	GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);

	//-------------------------------------------------------------------------------------//
	// 初始化紋理矩陣以及區域成長判定的矩陣大小和初始值
	//
	register int i, j;
	int totalx = ROW * COL;
	int totaly = Total_Slice;
	judge = New2Dmatrix(Total_Slice, ROW * COL, short);

	for (j = 0; j < totaly; j++)
	{
		for (i = 0; i < totalx; i++)
		{
			judge[j][i] = 0;
		}
	}

	for (j = 0; j < 256*256*256; j++)
	{
		for (i = 0; i < 4; i++)
		{
			m_image0[j][i] = 0;
		}
	}
	
	//-------------------------------------------------------------------------------------//
	// 更改三維影像Z軸方向的縮放比例(不要懷疑，改的是Z軸沒錯)
	//
	if (totaly < 200)
		scale_x = 0.25F;
	else if (totaly >= 200 && totaly < 300)
		scale_x = 0.3F;
	else if (totaly >= 300 && totaly < 350)
		scale_x = 0.35F;
	else if (totaly > 350)
		scale_x = 0.4F;
	
	//-------------------------------------------------------------------------------------//
	// 設定校正參數
	//
	x_index = (2.0F / 511.0F) * (COL / 2.0F) / 2;
	x_index = x_index / scale_y;
	y_index = (2.0F / 511.0F) * (ROW / 2.0F) / 2;
	y_index = y_index / scale_z;
	z_index = (2.0F / 511.0F) * (Total_Slice / 2.0F) / 2;
	z_index = z_index / scale_x;

	//-------------------------------------------------------------------------------------//
	// openGL空間建立
	//
	m_hDC = ::GetDC(m_3D_frame->m_hWnd);					// 獲得畫布物件DC的HANDLE（hDC）
	SetupPixelFormat(m_hDC);
	if ((m_hRC = ::wglCreateContext(m_hDC)) == 0)			// 產生 openGL 所需的畫布（hRC）
	{
		AfxMessageBox("Fail to create hRC context!");
		return FALSE;
	}
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)			// 建立 hDC 與 hRC 之間的連結
	{
		AfxMessageBox("Fail to make current!");
		return FALSE;
	}
	PerspectiveBuild();										// 建立openGL透視空間
	gl_3DTexture = ExtensionSupported("GL_EXT_texture3D");	// 確認是否支援3D紋理
	if (gl_3DTexture == TRUE)
	{
		// Return the address of an openGL extension function.
		glTexImage3D = (PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D");
		Mat_Offset = (512 - Total_Slice) / 2;
		
		GLInitialization();
	}
	else
	{
		AfxMessageBox("This program requires 3D Texture support!");
		return FALSE;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void C3DProcess::OnPaint()
{
	// 繪圖事件
	//
	CPaintDC dc(this); // device context for painting
					   // TODO: Add your message handler code here
					   // Do not call CDialogEx::OnPaint() for painting messages
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
		return;
	}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	Draw2DImage(DisplaySlice);
	Draw3DImage(true);
	get_3Dimage = true;
}

BOOL C3DProcess::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠滾輪事件
	//
	POINT mpt;						// 獲得鼠標當下位置
	GetCursorPos(&mpt);
	ScreenToClient(&mpt);
	int TotalSlice = Total_Slice;

	// 在 二維 影像視窗範圍內
	//
	if (mpt.x < m_2D_rect.right && mpt.x > m_2D_rect.left && mpt.y < m_2D_rect.bottom && mpt.y > m_2D_rect.top)
	{
		if (zDelta < 0)
			DisplaySlice += 1;
		else if (zDelta > 0 && DisplaySlice > 0)
			DisplaySlice -= 1;

		if (DisplaySlice >= TotalSlice)
			DisplaySlice = TotalSlice - 1;
		else if (DisplaySlice < 0)
			DisplaySlice = 0;

		Draw2DImage(DisplaySlice);
		m_ScrollBar.SetScrollPos(DisplaySlice);
	}

	// 在 三維 影像視窗範圍內
	//
	if (mpt.x < m_3D_rect.right && mpt.x > m_3D_rect.left && mpt.y < m_3D_rect.bottom && mpt.y > m_3D_rect.top)
	{
		if (mode == ControlModes::ControlObject)
		{
			if (zDelta < 0)
				viewDistance += 0.05F;
			else if (zDelta > 0)
				viewDistance -= 0.05F;
		}
		else if (mode == ControlModes::ControlPlane)
		{
			if (zDelta < 0)
				user_Plane[3] += 0.01F;
			else if (zDelta > 0)
				user_Plane[3] -= 0.01F;
		}
		Draw3DImage(true);
	}

	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void C3DProcess::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default
	// ScrollBar 垂直捲動事件
	//
	int n = m_ScrollBar.GetScrollPos();			// n 等同於 slice
	int TotalSlice = Total_Slice;

	switch (nSBCode)
	{
	case SB_LINEUP :
		n -= 1;
		break;
	case SB_LINEDOWN :
		n += 1;
		break;
	case SB_PAGEUP :
		n -= 5;
		break;
	case SB_PAGEDOWN :
		n += 5;
		break;
	case SB_TOP : 
		n = 0;
		break;
	case SB_BOTTOM : 
		n = TotalSlice - 1;
		break;
	case SB_THUMBPOSITION :
		n = nPos;
		break;
	case SB_THUMBTRACK :
		n = nPos;
		break;
	}

	if (n >= TotalSlice)
		n = TotalSlice - 1;
	else if (n < 0)
		n = 0;
	
	m_ScrollBar.SetScrollPos(n);
	DisplaySlice = n;
	Draw2DImage(DisplaySlice);

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

void C3DProcess::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠移動事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		if (nFlags == MK_LBUTTON || nFlags == MK_RBUTTON)
		{
			ActTracking(point.x - m_3D_rect.left, point.y - m_3D_rect.top);

			Draw3DImage(true);
		}
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void C3DProcess::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠左鍵 按下 事件
	//
	// 三維 影像視窗
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		ActStart(nFlags, point.x - m_3D_rect.left, point.y - m_3D_rect.top);

		if (m_3Dseed == TRUE)
		{
			if (get_3Dseed == false)
			{
				GLfloat		win_x, win_y, win_z;
				GLdouble	obj_x, obj_y, obj_z;

				GLint*		viewPort = new GLint[16];
				GLdouble*	modelView_matrix = new GLdouble[16];
				GLdouble*	projection_matrix = new GLdouble[16];

				glGetIntegerv(GL_VIEWPORT, viewPort);
				glGetDoublev(GL_MODELVIEW_MATRIX, modelView_matrix);
				glGetDoublev(GL_PROJECTION_MATRIX, projection_matrix);

				win_x = (GLfloat)(point.x - m_3D_rect.left);
				win_y = (GLfloat)(viewPort[3] - (point.y - m_3D_rect.top));
				glReadPixels((int)win_x, (int)win_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &win_z);
				gluUnProject(win_x, win_y, win_z, modelView_matrix, projection_matrix, viewPort, &obj_x, &obj_y, &obj_z);

				seed_gl.x = obj_z;						// x_axis
				seed_gl.y = obj_y;						// y_axis
				seed_gl.z = obj_x;						// slice

				seed_img = coordiConvert(seed_gl);		// 座標點與資料矩陣位置的轉換

				if (seed_img.z >= 0 && seed_img.z <= Total_Slice - 1)
				{
					if (seed_img.y >= 0 && seed_img.y <= ROW - 1)
					{
						if (seed_img.x >= 0 && seed_img.x <= COL - 1)
						{
							get_3Dseed = true;
							DisplaySlice = seed_img.z;
							m_ScrollBar.SetScrollPos(DisplaySlice);

							GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(TRUE);
							GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(TRUE);

							//--------------------------------------------------------------------------//
							
							short Pos_5 = seed_img.x;
							short Pos_6 = seed_img.y;
							short Pos_7 = seed_img.z;
							short Pos_8 = m_pDoc->m_img[seed_img.z][seed_img.y * COL + seed_img.x];

							m_pos_5.Format("%d", (int)Pos_5);
							m_pos_6.Format("%d", (int)Pos_6);
							m_pos_7.Format("%d", (int)Pos_7);
							m_pos_8.Format("%d", (int)Pos_8);
						}
					}
				}
				delete[] viewPort;
				delete[] modelView_matrix;
				delete[] projection_matrix;

				UpdateData(FALSE);
				Draw3DImage(true);
				Draw2DImage(DisplaySlice);
			}
		}
	}
	// 二維 影像視窗
	//
	if (point.x < m_2D_rect.right && point.x > m_2D_rect.left && point.y < m_2D_rect.bottom && point.y > m_2D_rect.top)
	{
		seed_pt.x = (short)(point.x - m_2D_rect.left);
		seed_pt.y = (short)(point.y - m_2D_rect.top);
		seed_pt.z = DisplaySlice;

		m_pos_1.Format("%d", (int)seed_pt.x);
		m_pos_2.Format("%d", (int)seed_pt.y);
		m_pos_3.Format("%d", (int)seed_pt.z);
		if (m_disp_org)
			m_pos_4.Format("%d", (int)m_pDoc->m_img[seed_pt.z][(seed_pt.y * COL) + seed_pt.x]);
		else if (m_disp_pro0)
			m_pos_4.Format("%d", (int)m_pDoc->m_imgPro[seed_pt.z][(seed_pt.y * COL) + seed_pt.x]);

		// 觀察一下標準差
		std::vector<int> pixel;
		double avg = 0, sd = 0, cnt = 0;
		/*for (int k = -1; k <= 1; k++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					sum += m_pDoc->m_img[seed_pt.z + k][(seed_pt.y + j) * COL + seed_pt.x + i];
					cnt++;
				}
			}
		}
		avg = sum / cnt;
		for (int k = -1; k <= 1; k++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int i = -1; i <= 1; i++)
				{
					pixel = m_pDoc->m_img[seed_pt.z + k][(seed_pt.y + j) * COL + seed_pt.x + i];
					sd += pow((pixel - avg), 2);
				}
			}
		}
		sd = sqrt(sd / cnt);*/
		auto average = [&](Seed_s s, std::vector<int>& v)
		{	// 計算周圍像素平均
			register int i, j, k;
			for (k = -1; k <= 1; ++k)
			{
				for (j = -1; j <= 1; ++j)
				{
					for (i = -1; i <= 1; ++i)
					{
						//sum += imgPro[s.z + k][(s.y + j) * col + (s.x + i)];
						v.push_back(m_pDoc->m_imgPro[s.z + k][(s.y + j) * COL + (s.x + i)]);
						cnt += 1;
					}
				}
			}
			return (std::accumulate(v.begin(), v.end(), 0.0L) / cnt);
		};
		auto standard_deviation = [&](std::vector<int>& v, double AVG)
		{	// 計算周圍像素標準差
			double SD = 0;
			for (const auto& n : v)
			{
				SD += pow((n - AVG), 2);
			}
			return sqrt(SD / cnt);
		};
		avg = average(seed_pt, pixel);
		sd = standard_deviation(pixel, avg);

		TRACE3("Cnt = %f, Avg = %f, Sd = %f \n", cnt, avg, sd);
		TRACE1("Judge = %d \n", judge[seed_pt.z][seed_pt.y * COL + seed_pt.x]);

		if (m_3Dseed)
			GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(TRUE);

		get_2Dseed = true;
		UpdateData(FALSE);
		Draw2DImage(DisplaySlice);
	}
	CDialogEx::OnLButtonDown(nFlags, point);
}

void C3DProcess::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠左鍵 放開 事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		ActStop(nFlags, point.x - m_3D_rect.left, point.y - m_3D_rect.top);

		Draw3DImage(true);
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void C3DProcess::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠右鍵 按下 事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		ActStart(nFlags, point.x - m_3D_rect.left, point.y - m_3D_rect.top);
		
		Draw3DImage(true);
	}
	CDialogEx::OnRButtonDown(nFlags, point);
}

void C3DProcess::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠右鍵 放開 事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		ActStop(nFlags, point.x - m_3D_rect.left, point.y - m_3D_rect.top);
		
		Draw3DImage(true);
	}
	CDialogEx::OnRButtonUp(nFlags, point);
}

void C3DProcess::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠右鍵 按兩下 事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		if (mode == ControlModes::ControlObject)
		{
			OnBnClickedCheckPlane();
		}
		else
		{
			OnBnClickedCheckObject();
		}
	}
	CDialogEx::OnRButtonDblClk(nFlags, point);
}

void C3DProcess::OnEnChangeEditPixelThreshold()
{
	// 更換 二值化閾值 (Pixel)
	// EditBox : Pixel Threshold (m_pixelThreshold)
	//
	UpdateData(TRUE);
	PixelThreshold = atoi(m_pixelThreshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditHuThreshold()
{
	// 更換 二值化閾值 (HU)
	// EditBox : HU Threshold (m_HUThreshold)
	//
	UpdateData(TRUE);
	HUThreshold = atoi(m_HUThreshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditSlices()
{
	// 更換openGL繪製三維影像的紋理層數
	// Edit : vertex's slices (m_slices)
	//
	UpdateData(TRUE);
	glSlices = atoi(m_slices);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedCheckObject()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Object (m_object)
	//
	mode = ControlModes::ControlObject;
	m_object = TRUE;
	m_plane = FALSE;
	UpdateData(FALSE);
}

void C3DProcess::OnBnClickedCheckPlane()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Plane (m_plane)
	//
	mode = ControlModes::ControlPlane;
	m_object = FALSE;
	m_plane = TRUE;
	UpdateData(FALSE);
}

void C3DProcess::OnBnClickedCheckDispOrg()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D Image - Original (m_disp_org)
	//
	m_disp_org = TRUE;
	m_disp_pro0 = FALSE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
	GetDlgItem(IDC_CHECK_HU_THRESHOLD)->EnableWindow(TRUE);
}

void C3DProcess::OnBnClickedCheckDispPro0()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D Image - Processed (m_disp_pro0)
	//
	m_disp_org = FALSE;
	m_disp_pro0 = TRUE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
	GetDlgItem(IDC_CHECK_HU_THRESHOLD)->EnableWindow(FALSE);
}

void C3DProcess::OnBnClickedCheckComplete()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Complete (m_complete)
	//
	m_complete = TRUE;
	m_thresholdHU = FALSE;
	m_thresholdPixel = FALSE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheckPixelThreshold()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Threshold<Pixel> (m_thresholdPixel)
	//
	m_complete = FALSE;
	m_thresholdHU = FALSE;
	m_thresholdPixel = TRUE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheckHuThreshold()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Threshold<HU> (m_thresholdHU)
	//
	m_complete = FALSE;
	m_thresholdHU = TRUE;
	m_thresholdPixel = FALSE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheck3dSeed()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 3D_seed (m_3Dseed)
	//
	UpdateData(TRUE);
	if (m_3Dseed == TRUE)
	{
		if (get_2Dseed)
		{
			GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(TRUE);
		}
		if (get_3Dseed)
		{
			GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(TRUE);
		}
		if (get_regionGrow)
		{
			GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(TRUE);
		}
	}
	else
	{
		GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
	}
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedButtonIntensityPlus()
{
	// TODO: Add your control notification handler code here
	// Button : Intensity_Plus (m_intensity)
	//
	intensity += 1.0F / 32.0F;
	if (intensity > 1.0F)	intensity = 1.0F;
	m_intensity.Format("%f", intensity);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonIntensityMinus()
{
	// TODO: Add your control notification handler code here
	// Button : Intensity_Minus (m_intensity)
	//
	intensity -= 1.0F / 32.0F;
	if (intensity < 0.0F)	intensity = 0.0F;
	m_intensity.Format("%f", intensity);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonDensityPlus()
{
	// TODO: Add your control notification handler code here
	// Button : Density_Plus (m_density)
	//
	density += 1.0F / 32.0F;
	if (density > 1.0F)	density = 1.0F;
	m_density.Format("%f", density);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonDensityMinus()
{
	// TODO: Add your control notification handler code here
	// Button : Density_Minus (m_density)
	//
	density -= 1.0F / 32.0F;
	if (density < 0.0F)	density = 0.0F;
	m_density.Format("%f", density);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonSlicesPlus()
{
	// TODO: Add your control notification handler code here
	// Button : Slices_Plus (m_slices)
	//
	glSlices += 1;
	if (glSlices > 512)		glSlices = 512;
	m_slices.Format("%d", glSlices);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonSlicesMinus()
{
	// TODO: Add your control notification handler code here
	// Button : Slices_Minus (m_slices)
	//
	glSlices -= 1;
	if (glSlices < 0)	glSlices = 0;
	m_slices.Format("%d", glSlices);
	UpdateData(FALSE);
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonPlaneReset()
{
	// TODO: Add your control notification handler code here
	// Button : Plane Reset
	// (未完成...)
	pln_angle = obj_angle;
	pln_axis[0] = 0.0F;
	pln_axis[1] = 0.0F;
	pln_axis[2] = 0.0F;

	user_Plane[0] = 1.0L;
	user_Plane[1] = 0.0L;
	user_Plane[2] = 0.0L;
	user_Plane[3] = 1.0L;

	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButton2dseedClear()
{
	// TODO: Add your control notification handler code here
	// Button : 2D Seed Clear
	//
	if (get_2Dseed)
	{
		seed_pt.x = 0;
		seed_pt.y = 0;
		seed_pt.z = 0;

		m_pos_1.Format("%d", 0);
		m_pos_2.Format("%d", 0);
		m_pos_3.Format("%d", 0);
		m_pos_4.Format("%d", 0);

		get_2Dseed = false;
		if (m_3Dseed)
			GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(FALSE);
		
		UpdateData(FALSE);
		Draw2DImage(DisplaySlice);
	}
}

void C3DProcess::OnBnClickedButton3dseedClear()
{
	// TODO: Add your control notification handler code here
	// Button : 3D Seed Clear
	//
	if (get_3Dseed)
	{
		seed_gl.x = 0.0L;
		seed_gl.y = 0.0L;
		seed_gl.z = 0.0L;

		seed_img.x = 0;
		seed_img.y = 0;
		seed_img.z = 0;

		m_pos_5.Format("%d", 0);
		m_pos_6.Format("%d", 0);
		m_pos_7.Format("%d", 0);
		m_pos_8.Format("%d", 0);

		get_3Dseed = false;
		GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(FALSE);
		
		UpdateData(FALSE);
		Draw3DImage(true);
		Draw2DImage(DisplaySlice);
	}
}

void C3DProcess::OnBnClickedButtonSeedChange()
{
	// TODO: Add your control notification handler code here
	// Button : 2D -> 3D Seed
	//
	if (get_2Dseed)
	{
		seed_img = seed_pt;

		seed_gl.x = (double)(seed_pt.x / (COL / ((x_index + 1) - (-x_index + 1))) + (-x_index + 1) - 1);
		seed_gl.y = (double)(seed_pt.y / (ROW / ((y_index + 1) - (-y_index + 1))) + (-y_index + 1) - 1);
		seed_gl.z = (double)(seed_pt.z / (Total_Slice / ((z_index + 1) - (-z_index + 1))) + (-z_index + 1) - 1);

		m_pos_5.Format("%d", (int)seed_pt.x);
		m_pos_6.Format("%d", (int)seed_pt.y);
		m_pos_7.Format("%d", (int)seed_pt.z);
		if (m_disp_org)
			m_pos_8.Format("%d", (int)(m_pDoc->m_img[seed_pt.z][(seed_pt.y * 512) + seed_pt.x]));
		else if (m_disp_pro0)
			m_pos_8.Format("%d", (int)(m_pDoc->m_imgPro[seed_pt.z][(seed_pt.y * 512) + seed_pt.x]));

		get_3Dseed = true;
		DisplaySlice = seed_pt.z;
		GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(TRUE);

		UpdateData(FALSE);
		Draw3DImage(true);
		Draw2DImage(DisplaySlice);
	}
}

void C3DProcess::OnBnClickedButtonRegionGrowing()
{
	// TODO: Add your control notification handler code here
	// Button : 3D Region Growing
	//
	if (!get_3Dseed)	return;
	
	clock_t start, end;
	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("Region growing...");

	// 宣告 成長標準 參數
	//
	RG_totalTerm = {
		RG_totalTerm.seed = seed_img,
		RG_totalTerm.s_kernel = 3,
		RG_totalTerm.n_kernel = 3,
		RG_totalTerm.threshold = 50L,
		RG_totalTerm.coefficient = 2.0L
	};
	
	// 執行 3D_Region growing
	//
	start = clock();
	//RG_3D_GlobalAvgConnected(judge, RG_totalTerm);
	//RG_3D_LocalAvgConnected(judge, RG_totalTerm);
	RG_3D_ConfidenceConnected(judge, RG_totalTerm);
	end = clock();

	// 確認分割體積
	//
	get_regionGrow = true;
	RG_totalVolume = Calculate_Volume(judge);
	m_result.Format("%lf", RG_totalVolume);

	TRACE1("Growing Volume : %f (cm3) \n", RG_totalVolume);
	TRACE1("RG Time : %f (s) \n\n", (double)((end - start)) / CLOCKS_PER_SEC);
	
	GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(TRUE);
	
	PrepareVolume();
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
	m_wait->DestroyWindow();
	delete m_wait;
}

void C3DProcess::OnBnClickedButtonGrowingRemove()
{
	// TODO: Add your control notification handler code here
	// Button : Growing Remove
	// 把分割出來的部分(紅色標記)變透明
	//
	if (!get_regionGrow)	return;

	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;
	register int i, j, k;

	k = 0;
	while (k < 512)
	{
		if (k > sample_start && k <= sample_end)
		{
			for (j = 2; j < row - 2; j += 2)
			{
				for (i = 2; i < col - 2; i += 2)
				{
					if (judge[k - (Mat_Offset + 1)][j * col + i] > 0)
					{
						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							0, 0);
					}
				}
			}
		}
		k += 2;
	}
	LoadVolume();
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonGrowingRecovery()
{
	// TODO: Add your control notification handler code here
	// Button : Growing Recovery
	// 把變透明的部分變回原本的顏色標記
	//
	if (!get_regionGrow)	return;

	float pixel;
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;
	register int i, j, k;

	k = 0;
	while (k < 512)
	{
		if (k > sample_start && k <= sample_end)
		{
			for (j = 2; j < row - 2; j += 2)
			{
				for (i = 2; i < col - 2; i += 2)
				{
					pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * col + i];

					if (judge[k - (Mat_Offset + 1)][j * col + i] > 0)
					{
						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							pixel / 255.0F, 1);
					}
				}
			}
		}
		k += 2;
	}
	LoadVolume();
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonGrowingClear()
{
	// TODO: Add your control notification handler code here
	// Button : Growing Clear
	//
	if (!get_regionGrow)	return;

	m_result = _T("0.0");
	RG_totalVolume = 0.0L;
	get_regionGrow = false;
	GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(FALSE);
	//------------------------------------------------------------//

	float pixel;
	const int row = ROW;
	const int col = COL;
	const int totalx = ROW * COL;
	const int totaly = Total_Slice;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;

	// 恢復 : 成長判定矩陣
	//
	register int i, j, k;
	for (j = 0; j < totaly; j++)
	{
		for (i = 0; i < totalx; i++)
		{
			judge[j][i] = 0;
		}
	}

	// 恢復 : 影像
	//
	k = 0;
	while (k < 512)
	{
		if (k > sample_start && k <= sample_end)
		{
			for (j = 2; j < row - 2; j += 2)
			{
				for (i = 2; i < col - 2; i += 2)
				{
					pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * col + i];
					m_pDoc->m_imgPro[k - (Mat_Offset + 1)][j * col + i] = (BYTE)pixel;
					getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
						pixel / 255.0F, 0);
				}
			}
		}
		k += 2;
	}
	LoadVolume();
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedButtonDilation()
{
	// TODO: Add your control notification handler code here
	// Button : Dilation (目前為 未定型的處理)
	//
	if (!get_regionGrow)	return;

	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	BYTE**& org = m_pDoc->m_img;
	BYTE**& pro = m_pDoc->m_imgPro;
	clock_t start = clock();

	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("2nd Region growing...");

	// 尋找每張slice分割區域的垂直邊界 (y_min、x_min、x_max)
	// 
	std::map<int, std::vector<int>> edge;

	auto findBorder = [&](int start)
	{
		int position;
		int slice = start;
		std::vector<int> x_pos;
		std::vector<int> y_pos;

		while (slice < totalSlice)
		{
			// 尋找垂直邊界
			position = 0;
			while (position < totalXY)
			{
				if (judge[slice][position] == 1)
				{
					x_pos.push_back(position % col);
					y_pos.push_back(position / col);
				}
				position += 1;
			}
			if (x_pos.empty() || y_pos.empty())	continue;

			std::sort(x_pos.begin(), x_pos.end());
			std::sort(y_pos.begin(), y_pos.end());

			edge[slice].push_back(*(x_pos.begin()));		// [0]: x_min
			edge[slice].push_back(*(x_pos.end() - 1));		// [1]: x_max
			edge[slice].push_back(*(y_pos.begin()));		// [2]: y_min
			edge[slice].push_back(*(y_pos.end() - 1));		// [3]: y_max

			// 尋找三角頂點
			vertex[slice].assign(3, std::make_pair(0, 0));
			int cur = edge[slice].at(0) + edge[slice].at(2) * col;
			int end = edge[slice].at(1) + edge[slice].at(3) * col;
			bool l = false, m = false, r = false;
			while (cur < end)
			{
				if (judge[slice][cur] == 1)
				{
					if (!m && (cur / col) == edge[slice].at(2))	// 中上
					{
						vertex[slice][0] = std::make_pair((cur % col), (cur / col));
						m = true;
					}	
					if (!l && (cur % col) == edge[slice].at(0))	// 左下
					{
						vertex[slice][1] = std::make_pair((cur % col), (cur / col));
						l = true;
					}
					if (!r && (cur % col) == edge[slice].at(1))	// 右下
					{
						vertex[slice][2] = std::make_pair((cur % col), (cur / col));
						r = true;
					}
				}
				cur += 1;
			}

			// 計算每張slice三角頂點的斜線方程式係數
			line[slice].assign(2, std::make_pair(0.0f, 0.0f));
			float slope1 = 0, slope2 = 0;						// 斜率
			float inter1 = 0, inter2 = 0;						// 截距
			slope1 = (float)(vertex[slice][0].second - vertex[slice][1].second) / 
					(float)(vertex[slice][0].first - vertex[slice][1].first);
			slope2 = (float)(vertex[slice][0].second - vertex[slice][2].second) / 
					(float)(vertex[slice][0].first - vertex[slice][2].first);

			inter1 = (float)(vertex[slice][0].second + vertex[slice][1].second) -
						slope1 * (vertex[slice][0].first + vertex[slice][1].first);
			inter1 /= 2;
			inter2 = (float)(vertex[slice][0].second + vertex[slice][2].second) -
						slope2 * (vertex[slice][0].first + vertex[slice][2].first);
			inter2 /= 2;
			line[slice][0] = std::make_pair(slope1, inter1);	// left line
			line[slice][1] = std::make_pair(slope2, inter2);	// right line

			x_pos.clear();	x_pos.shrink_to_fit();
			y_pos.clear();	y_pos.shrink_to_fit();
			slice += 2;
		}
	};

	thread th0(findBorder, 0);				// 偶數 slice
	thread th1(findBorder, 1);				// 奇數 slice
	th0.join();	th1.join();

	// 針對 分割範圍的方形區域 做pixel的處理
	//
	auto edgeProcess = [&](int start)
	{
		std::map<int, std::vector<int>>::iterator iter;
		int slice = start;
		while (slice < totalSlice)
		{
			//iter = edge.find(slice);
			if ((iter = edge.find(slice)) != edge.end())
			{
				for (int j = iter->second.at(2); j <= iter->second.at(3); j++)
				{
					for (int i = iter->second.at(0); i <= iter->second.at(1); i++)
					{
						if (pro[slice][j * col + i] <= 100)
							pro[slice][j * col + i] = 0;
						else if (pro[slice][j * col + i] <= 170)
							pro[slice][j * col + i] -= 20;
						else if (pro[slice][j * col + i] <= 220 && pro[slice][j * col + i] > 180)
							pro[slice][j * col + i] += 30;
					}
				}
			}
			slice += 2;
		}
	};

	thread th2(edgeProcess, 0);
	thread th3(edgeProcess, 1);
	th2.join();	th3.join();

	// 低通 濾波 (mean filter)
	//
	std::vector<int> avg_coef{ 1, 2, 1, 2, 4, 2, 1, 2, 1 };
	//std::vector<int> avg_coef{ 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	int cnt = std::accumulate(avg_coef.begin(), avg_coef.end(), 0);

	auto avgKernel = [&](int slice, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; j++)
		{
			for (int i = x - 1; i <= x + 1; i++)
			{
				sum += (avg_coef[n] * pro[slice][j * col + i]);
				n += 1;
			}
		}
		return sum / cnt;
	};

	auto avgFilter = [&](int start)
	{
		std::map<int, std::vector<int>>::iterator iter;
		int slice = start;
		while (slice < totalSlice)
		{
			//iter = edge.find(slice);
			if ((iter = edge.find(slice)) != edge.end())
			{
				for (int j = iter->second.at(2) + 1; j < iter->second.at(3); j++)
				{
					for (int i = iter->second.at(0) + 1; i < iter->second.at(1); i++)
					{
						pro[slice][j * col + i] = avgKernel(slice, i, j);
					}
				}
			}
			slice += 2;
		}
	};

	thread th4(avgFilter, 0);
	thread th5(avgFilter, 1);
	th4.join();	th5.join();

	// 銳化 濾波 (high-boost filter)
	//
	float weighted = -2.5f;
	auto sharpFilter = [&](int start)
	{
		std::map<int, std::vector<int>>::iterator iter;
		int slice = start;
		int pixel = 0;
		while (slice < totalSlice)
		{
			//iter = edge.find(slice);
			if ((iter = edge.find(slice)) != edge.end())
			{
				for (int j = iter->second.at(2); j <= iter->second.at(3); j++)
				{
					for (int i = iter->second.at(0); i <= iter->second.at(1); i++)
					{
						pixel = (int)(org[slice][j * col + i] +
							weighted * (org[slice][j * col + i] - pro[slice][j * col + i]));
						pixel = (pixel > 255) ? 255 : pixel;
						pixel = (pixel < 0) ? 0 : pixel;
						pro[slice][j * col + i] = pixel;
					}
				}
			}
			slice += 2;
		}
	};

	thread th6(sharpFilter, 0);
	thread th7(sharpFilter, 1);
	th6.join();	th7.join();

	//////////////////////////////////////////////////////////
	//
	//     二 次 區 域 成 長
	//
	RG_totalTerm = {
		RG_totalTerm.seed = seed_img,
		RG_totalTerm.s_kernel = 3,
		RG_totalTerm.n_kernel = 3,
		RG_totalTerm.threshold = 50L,
		RG_totalTerm.coefficient = 0.5L
	};

	RG2_3D_ConfidenceConnected(judge, RG_totalTerm);
	clock_t end = clock();
	
	PrepareVolume();
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
	RG_totalVolume = Calculate_Volume(judge);
	
	m_result.Format("%lf", RG_totalVolume);
	TRACE1("Growing Volume : %f (cm3)\n", RG_totalVolume);
	TRACE1("2nd process Time : %f (s)\n\n", (double)(end - start) / CLOCKS_PER_SEC);
	m_wait->DestroyWindow();
	UpdateData(FALSE);
	delete m_wait;
}

//==========================//
//   C3DProcess Functions   //
//==========================//

BOOL C3DProcess::SetupPixelFormat(HDC hDC)
{
	// DO : 設置(初始化)hDC的像素結構
	//
	PIXELFORMATDESCRIPTOR pfd = {									// pfd, Pixel Format Descriptor
		sizeof(PIXELFORMATDESCRIPTOR),								// size of this pfd
		1,															// version number
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,	// support window, opengl, double buffer
		PFD_TYPE_RGBA,												// RGBA type
		24,															// 24-bit color depth
		0,0,0,0,0,0,												// color bits ignored
		0,															// no alpha buffer
		0,															// shift bit ignored
		0,															// no accumulation buffer
		0,0,0,0,													// accum bits ignored
		16,															// 16-bit z-buffer
		0,															// no stencil buffer
		0,															// no auxiliary buffer
		PFD_MAIN_PLANE,												// main layer
		0,															// reserved
		0,0,0														// layer masks ignored
	};

	// 設置像素格式
	//
	int pixelformat;
	if ((pixelformat = ::ChoosePixelFormat(hDC, &pfd)) == 0)
	{
		AfxMessageBox("Fail to choose pixel format!");
		return FALSE;
	}
	if (::SetPixelFormat(hDC, pixelformat, &pfd) == FALSE)
	{
		AfxMessageBox("Fail to set pixel format!");
		return FALSE;
	}

	// 測試像素格式
	//
	int npixelformat;
	npixelformat = GetPixelFormat(hDC);
	::DescribePixelFormat(hDC, npixelformat, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if (!(pfd.dwFlags & PFD_NEED_PALETTE))
		return NULL;

	return TRUE;
}

BOOL C3DProcess::ExtensionSupported(const char* exten)
{
	// DO : 擴展支援確認
	//
	static const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *)strchr(exten, ' ');
	if (where || *exten == '\0')
		return false;

	if (!extensions)
		extensions = glGetString(GL_EXTENSIONS);

	start = extensions;
	for (;;)
	{
		where = (GLubyte *)strstr((const char *)start, exten);
		if (!where)
			break;
		terminator = where + strlen(exten);
		if (where == start || *(where - 1) == ' ')
		{
			if (*terminator == ' ' || *terminator == '\0')
			{
				return true;
			}
		}
		start = terminator;
	}
	return false;
}

void C3DProcess::PerspectiveBuild()
{
	// DO : 建立openGL透視空間
	//
	GLint	gl_x, gl_y;
	gl_x = m_3D_rect.right - m_3D_rect.left;
	gl_y = m_3D_rect.bottom - m_3D_rect.top;

	glViewport(0, 0, gl_x, gl_y);									// 設定螢幕視窗窗口顯示範圍
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();												// 重制當前矩陣轉為單位矩陣
	gluPerspective(40.0, (GLfloat)gl_x/(GLfloat)gl_y, 0.5, 10.0);	// 建立對稱的透視視覺空間
	glMatrixMode(GL_MODELVIEW);
}

void C3DProcess::GLInitialization()
{
	// DO : openGL繪圖初始化
	//
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
		return;
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	 
	// 建立3D紋理儲存空間
	//
	glGenTextures(ImageFrame, textureName);					// 告訴 openGL 配置一塊記憶體空間存放材質
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);					// 控制 讀取(or傳輸) 像素數據對齊方式 ↓
															// 預設 4 bytes，設成 1 byte 避免 padding
	// 載入紋理
	//
	PrepareVolume();

	// 建立頂點座標(建立openGL繪圖時，glVertex()所需要的點)
	//
	register int i, j;
	float temp = 0.0f;

	for (i = 0; i < 8; i++)
	{
		for (j = 0; j < 8; j++)
		{
			glVertexPt[i * 8 + j][0] = (float)(j / 7.0f) - 0.5f;
			glVertexPt[i * 8 + j][1] = (float)(i / 7.0f) - 0.5f;
			glVertexPt[i * 8 + j][2] = -0.5f;
			
			temp = glVertexPt[i * 8 + j][0] * glVertexPt[i * 8 + j][0] +
				glVertexPt[i * 8 + j][1] * glVertexPt[i * 8 + j][1] +
				glVertexPt[i * 8 + j][2] * glVertexPt[i * 8 + j][2];
			temp = 1.0f / (float)sqrt(temp);
			
			glVertexPt[i * 8 + j][0] *= temp;
			glVertexPt[i * 8 + j][1] *= temp;
			glVertexPt[i * 8 + j][2] *= temp;
			
			glVertexPt[i * 8 + j][0] *= 2.0f;
			glVertexPt[i * 8 + j][1] *= 2.0f;
			glVertexPt[i * 8 + j][2] += 1.0f;
		}
	}
}

void C3DProcess::PrepareVolume()
{
	// DO : 建立紋理的資料矩陣
	//
	if (m_pDoc->m_img == NULL)	return;

	// 預備要用來建立紋理的資料矩陣
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;
	
	float pixel = 0.0F;
	register int i, j, k;

	CProgress* m_progress = new CProgress();
	m_progress->Create(IDD_DIALOG_PROGRESSBAR);
	m_progress->ShowWindow(SW_NORMAL);
	m_progress->SetInitial(0, 1, Total_Slice/2);
	m_progress->SetStatic("Construct 3D Image...");

	i = 0;	j = 0;	k = 0;
	
	if (!get_3Dimage)
	{
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				for (j = 2; j < row - 2; j += 2)
				{
					for (i = 2; i < col - 2; i += 2)
					{
						pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * col + i];

						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							pixel / 255.0F, 0);
					}
				}
			}
			k += 2;
			m_progress->Run();
		}
	}
	else
	{
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				for (j = 2; j < row - 2; j += 2)
				{
					for (i = 2; i < col - 2; i += 2)
					{
						pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * col + i];

						if (judge[k - (Mat_Offset + 1)][j * col + i] <= 0)
						{
							getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								pixel / 255.0F, 0);
						}
						else
						{
							getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								pixel / 255.0F, 1);
						}
					}
				}
			}
			k += 2;
			m_progress->Run();
		}
	}
	
	LoadVolume();
	m_progress->DestroyWindow();
	delete m_progress;
}

void C3DProcess::LoadVolume()
{
	// DO : 建立3D紋理
	//
	if (gl_3DTexture)
	{
		float color[] = { 1.0F, 1.0F, 1.0F, 1.0F };

		// 紋理座標系統：S為橫軸、T為縱軸、R為插入螢幕的軸
		// 設置紋理環繞模式
		//
		glBindTexture(GL_TEXTURE_3D, textureName[0]);							// 綁定紋理（指定類型, 紋理對象ID）
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// 紋理過濾函數（選擇濾鏡）
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// 放大時的濾鏡方式
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		// 縮小時的濾鏡方式
		glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, (GLvoid*)m_image0);								// 創建3D紋理
	}
}

void C3DProcess::getRamp(GLubyte* color, float t, int n)
{
	// DO : 計算RGBA的數值
	//
	if (n == 0)							// Gray Scale
	{
		color[0] = (GLubyte)(255 * t);	// R
		color[1] = (GLubyte)(255 * t);	// G
		color[2] = (GLubyte)(255 * t);	// B
		color[3] = (GLubyte)(60 * t);	// A
	}
	else if (n == 1)					// Red
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = 0;
		color[2] = 0;
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 2)					// Green
	{
		color[0] = 0;
		color[1] = (GLubyte)(255 * t);
		color[2] = 0;
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 3)					// Blue
	{
		color[0] = 0;
		color[1] = 0;
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 4)					// Orange
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = (GLubyte)(100 * t);
		color[2] = (GLubyte)(50 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 5)					// Purple
	{
		color[0] = (GLubyte)(255 * t);
		color[1] = 0;
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
	else if (n == 6)					// Water Blue
	{
		color[0] = 0;
		color[1] = (GLubyte)(255 * t);
		color[2] = (GLubyte)(255 * t);
		color[3] = (GLubyte)(255 * t);
	}
}

void C3DProcess::Draw3DImage(bool which)
{
	// DO : 繪製 3D 影像
	//
	CClientDC dc(m_3D_frame);

	// openGL 默認 4X4 矩陣與記憶體 1D 陣列關係
	// ┌				 ┐
	// ∣	 m0	 m4  m8  m12 ∣	
	// ∣ m1	 m5  m9  m13 ∣
	// ∣	 m2  m6  m10 m14 ∣
	// ∣ m3  m7  m11 m15 ∣
	// └				 ┘
	// (m12, m13, m14)是用作 Translation，(m15)是齊次座標（用作 Projection），
	// 左上 9 個元素用作 Rotate 和 Scale。

	// Xform matrices(旋轉後的模型矩陣)
	//
	static float objectXform[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	static float planeXform[16] =
	{
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};

	// clip planes equation（A, B, C, Z）
	// Ax + By + Cz + D = 0，如果是(0, -1, 0, 0)，
	// 意思是 y<0 的才能顯示。
	//
	double clip0[] = {-1.0,  0.0,  0.0, 1.0 };
	double clip1[] = { 1.0,  0.0,  0.0, 1.0 };
	double clip2[] = { 0.0, -1.0,  0.0, 1.0 };
	double clip3[] = { 0.0,  1.0,  0.0, 1.0 };
	double clip4[] = { 0.0,  0.0, -1.0, 1.0 };
	double clip5[] = { 0.0,  0.0,  1.0, 1.0 };

	// Texgen planes
	//
	float xPlane[] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float yPlane[] = { 0.0f, 1.0f, 0.0f, 0.0f };
	float zPlane[] = { 0.0f, 0.0f, 1.0f, 0.0f };

	float mat[16];
	float temp[3];
	float plane[12];

	int clip;
	register int i, j, k;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);								// 啟動深度測試(沒有開啟的話，整個物件會有點透明)

	// 建立觀看物件的 透明視景體、視角方向、距離(viewDistance)
	//
	gluPerspective(90, 1, 1, 700);							// 關掉(註解掉)這個，3D seed 的效果就會不好啊~
	glMatrixMode(GL_MODELVIEW);								// 模型視圖矩陣
	glLoadIdentity();										// testmat[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
	glTranslatef(0.0f, 0.0f, viewDistance);					// testmat[1,0,0,0, 0,1,0,0, 0,0,1,-4, 0,0,0,1]

	// 控制 心臟 旋轉
	//
	if (mode == ControlModes::ControlObject)
	{
		glPushMatrix();
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(obj_angle, obj_axis[0], obj_axis[1], obj_axis[2]);
			glMultMatrixf((GLfloat *)objectXform);
			glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)objectXform);
		}
		glPopMatrix();
	}
	glMultMatrixf((GLfloat *)objectXform);					// 關於物體(心臟)旋轉，刪除後就沒有辦法旋轉了

	// 控制 辣個平面 旋轉
	//
	if ((mode == ControlModes::ControlPlane) && (Act_Rotate == true))
	{
		// handle the plane rotations
		temp[0] = objectXform[0] * pln_axis[0] + objectXform[4] * pln_axis[1] + objectXform[8] * pln_axis[2];
		temp[1] = objectXform[1] * pln_axis[0] + objectXform[5] * pln_axis[1] + objectXform[9] * pln_axis[2];
		temp[2] = objectXform[2] * pln_axis[0] + objectXform[6] * pln_axis[1] + objectXform[10] * pln_axis[2];

		glPushMatrix();
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glRotatef(pln_angle, temp[0], temp[1], temp[2]);
			glMultMatrixf((GLfloat *)planeXform);
			glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)planeXform);
		}
		glPopMatrix();
	}

	// 只需要 Rotation，所以將 Translation 設為 0
	//
	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	InvertMat(mat);

	mat[12] = 0.0F;
	mat[13] = 0.0F;
	mat[14] = 0.0F;

	// 獲得 user_Plane 的平面方程式係數
	// 讓 辣個平面 在自身坐標系的 x 軸方向位移時，
	// 讓 物件(心臟) 隨 辣個平面 的 x 軸 移動方向解剖（切平面）。
	//
	user_Plane[0] = -planeXform[0];
	user_Plane[1] = -planeXform[1];
	user_Plane[2] = -planeXform[2];

	// 自動生成紋理座標系
	//
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

	// 建立裁剪平面(Clip plane)的四個角落的點
	// 計算 clip plane 四個角落的點 plane[16]={1,-1,-1, 1,-1,1, 1,1,-1, 1,1,1}
	//
	for (i = 0; i < 4; i++)
	{
		plane[i * 3 + 0] = (float)(planeXform[0] * user_Plane[3]);
		plane[i * 3 + 1] = (float)(planeXform[1] * user_Plane[3]);
		plane[i * 3 + 2] = (float)(planeXform[2] * user_Plane[3]);
		plane[i * 3 + 0] += (float)(planeXform[4] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 1] += (float)(planeXform[5] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 2] += (float)(planeXform[6] * ((i < 2) ? -1.0f : 1.0f));
		plane[i * 3 + 0] += (float)(planeXform[8] * ((i & 0x1) ? 1.0f : -1.0f));
		plane[i * 3 + 1] += (float)(planeXform[9] * ((i & 0x1) ? 1.0f : -1.0f));
		plane[i * 3 + 2] += (float)(planeXform[10] * ((i & 0x1) ? 1.0f : -1.0f));
	}

	// 尋找與觀測者相反的裁剪平面
	//
	if (fabs(objectXform[2]) > fabs(objectXform[6]))
	{
		if (fabs(objectXform[2]) > fabs(objectXform[10]))
		{
			if (objectXform[2] > 0.0f)		// X is largest
				clip = 1;					// positive
			else
				clip = 0;					// negative
		}
		else 
		{
			if (objectXform[10] > 0.0f)		// Z is largest
				clip = 5;					// positive
			else
				clip = 4;					// negative
		}
	}
	else
	{
		if (fabs(objectXform[6]) > fabs(objectXform[10]))
		{
			if (objectXform[6] > 0.0f)		// Y is largest
				clip = 3;					// positive
			else
				clip = 2;					// negative
		}
		else
		{
			if (objectXform[10] > 0.0f)		// Z is largest
				clip = 4;					// positive
			else
				clip = 5;					// negative
		}
	}

	// 配置裁剪平面 0 ~ 5
	//
	glClipPlane(GL_CLIP_PLANE0, clip0);
	glClipPlane(GL_CLIP_PLANE1, clip1);
	glClipPlane(GL_CLIP_PLANE2, clip2);
	glClipPlane(GL_CLIP_PLANE3, clip3);
	glClipPlane(GL_CLIP_PLANE4, clip4);
	glClipPlane(GL_CLIP_PLANE5, clip5);

	// 將與觀測者相反的裁剪平面設定為觀測者操作的 辣個平面
	//
	glClipPlane(GL_CLIP_PLANE0 + clip, user_Plane);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
	glEnable(GL_CLIP_PLANE4);
	glEnable(GL_CLIP_PLANE5);

	glColor4f(1.0F, 1.0F, 1.0F, intensity);				// 設定openGL的slcies顏色（R, G, B, Alpha）

	// 啟動 Alpha / 混合 測試
	//
	glEnable(GL_ALPHA_TEST);							// 啟用 Alpha 測試
	glAlphaFunc(GL_GREATER, density*intensity);			// 設定 Alpha 測試的參考值

	glEnable(GL_BLEND);									// 啟用 顏色混合
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// 設定 顏色混合(Source and Target)

	// 設定紋理座標系統
	//
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glEnable(GL_TEXTURE_3D);

	glTranslatef(0.5F, 0.5F, 0.5F);						// 將物件(心臟)移到正中心
	glScalef(scale_x, scale_y, scale_z);				// 將物件(心臟)縮放

	glMultMatrixf(mat);
	glTranslatef(0.0F, 0.0F, -viewDistance);

	// set modelView to identity
	//
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	{
		glLoadIdentity();

		// 設定紋理座標系統的生成
		//
		glTexGenfv(GL_S, GL_EYE_PLANE, xPlane);
		glTexGenfv(GL_T, GL_EYE_PLANE, yPlane);
		glTexGenfv(GL_R, GL_EYE_PLANE, zPlane);

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);

		glBindTexture(GL_TEXTURE_3D, textureName[0]);

		glTranslatef(0.0f, 0.0f, viewDistance);

		// 繪製openGL的每一層slices
		//
		for (k = 0; k < glSlices; k++)
		{
			glPushMatrix();
			{
				glTranslatef(0.0f, 0.0f, -1.0f + (float)k * (2.0f / (float)(glSlices - 1)));
				glBegin(GL_QUADS);
				{
					for (j = 0; j < (8 - 1); j++)
					{
						for (i = 0; i < (8 - 1); i++)
						{
							glVertex3fv(glVertexPt[j * 8 + i]);
							glVertex3fv(glVertexPt[j * 8 + (i + 1)]);
							glVertex3fv(glVertexPt[(j + 1) * 8 + (i + 1)]);
							glVertex3fv(glVertexPt[(j + 1) * 8 + i]);
						}
					}
				}
				glEnd();
			}
			glPopMatrix();
		}
		glDisable(GL_CLIP_PLANE0);
		glDisable(GL_CLIP_PLANE1);
		glDisable(GL_CLIP_PLANE2);
		glDisable(GL_CLIP_PLANE3);
		glDisable(GL_CLIP_PLANE4);
		glDisable(GL_CLIP_PLANE5);
	}
	glPopMatrix();

	// draw the slice plane across to get a better image
	//
	glDepthMask(GL_FALSE);
	glBegin(GL_QUADS);
	{
		glVertex3fv(&plane[0]);
		glVertex3fv(&plane[3]);
		glVertex3fv(&plane[6]);
		glVertex3fv(&plane[9]);
	}
	glEnd();
	glDepthMask(GL_TRUE);

	glDisable(GL_ALPHA_TEST);
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);

	// 繪製空間周圍邊框
	//
	glLineWidth(2);
	glBegin(GL_LINES);
	{
		glColor3f(0.902f, 1.0f, 1.0f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);

		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);

		glVertex3f(1.0f, 1.0f, 1.0f);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glVertex3f(1.0f, 1.0f, -1.0f);
		glVertex3f(-1.0f, 1.0f, -1.0f);

		glVertex3f(1.0f, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glVertex3f(-1.0f, -1.0f, -1.0f);
	}
	glEnd();

	// 繪製觀測者的平面
	//
	glLineWidth(1);
	glBegin(GL_LINES);
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3fv(&plane[0]);
		glVertex3fv(&plane[3]);

		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3fv(&plane[3]);
		glVertex3fv(&plane[9]);

		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3fv(&plane[9]);
		glVertex3fv(&plane[6]);

		glColor3f(1.0f, 1.0f, 0.0f);
		glVertex3fv(&plane[6]);
		glVertex3fv(&plane[0]);
	}
	glEnd();

	// 顯示 3D seed
	//
	if (m_3Dseed == TRUE && get_3Dseed == true)
	{
		glPointSize(5);
		glBegin(GL_POINTS);
		{
			glColor3f(1.0f, 0.0f, 0.0f);
			glVertex3f((GLfloat)seed_gl.z,
						(GLfloat)seed_gl.y,
						(GLfloat)seed_gl.x);
		}
		glEnd();
	}

	SwapBuffers(m_hDC);
}

void C3DProcess::Draw2DImage(unsigned short& slice)
{
	//	DO : 繪製 2D 影像
	//
	CClientDC dc(m_2D_frame);

	const int Row = ROW;
	const int Col = COL;
	register int i, j;

	if (m_pDoc->m_img == nullptr || m_pDoc->m_imgPro == nullptr)
	{
		AfxMessageBox("No Image Data can be display!!");
		return;
	}
	
	// 顯示 原始影像
	if (m_disp_org == TRUE)
	{	
		if (m_complete == TRUE)
		{
			m_2D_dib->ShowInverseDIB(&dc, m_pDoc->m_img[slice]);
		}
		// 顯示 以 Pixel 為閾值的二值化影像
		else if (m_thresholdPixel == TRUE)
		{
			PBYTE image_thres = new BYTE[Row * Col];

			i = 0;
			while (i < Row * Col)
			{
				if (m_pDoc->m_img[slice][i] > PixelThreshold)
					image_thres[i] = 255;
				else
					image_thres[i] = 0;
				i += 1;
			}
			m_2D_dib->ShowInverseDIB(&dc, image_thres);
			delete[] image_thres;
		}
		// 顯示 以 HU 為閾值的二值化影像
		else if (m_thresholdHU == TRUE)
		{
			PBYTE image_thres = new BYTE[Row * Col];

			i = 0;
			while (i < Row * Col)
			{
				if (m_pDoc->m_HUimg[slice][i] > HUThreshold)
					image_thres[i] = 255;
				else
					image_thres[i] = 0;
				i += 1;
			}
			m_2D_dib->ShowInverseDIB(&dc, image_thres);
			delete[] image_thres;
		}
	}
	// 顯示 處理過後的影像
	else if (m_disp_pro0 == TRUE)
	{
		if (m_complete == TRUE)
		{
			m_2D_dib->ShowInverseDIB(&dc, m_pDoc->m_imgPro[slice]);
		}
		// 顯示 以 Pixel 為閾值的二值化影像
		else if (m_thresholdPixel == TRUE)
		{
			PBYTE image_thres = new BYTE[Row * Col];

			i = 0;
			while (i < Row * Col)
			{
				if (m_pDoc->m_imgPro[slice][i] > PixelThreshold)
					image_thres[i] = 255;
				else
					image_thres[i] = 0;
				i += 1;
			}
			m_2D_dib->ShowInverseDIB(&dc, image_thres);
			delete[] image_thres;
		}
		// 顯示 以 HU 為閾值的二值化影像
		else if (m_thresholdHU == TRUE)
		{
			PBYTE image_thres = new BYTE[Row * Col];

			i = 0;
			while (i < Row * Col)
			{
				if (m_pDoc->m_HUimg[slice][i] > HUThreshold)
					image_thres[i] = 255;
				else
					image_thres[i] = 0;
				i += 1;
			}
			m_2D_dib->ShowInverseDIB(&dc, image_thres);
			delete[] image_thres;
		}
	}

	// 於2D影像顯示在2D點選的種子點
	if (get_2Dseed)
	{
		if (slice == (unsigned short)seed_pt.z)
		{
			CPoint pt;
			for (i = -1; i <= 1; i++)
			{
				for (j = -1; j <= 1; j++)
				{
					pt.x = (LONG)seed_pt.x + j;
					pt.y = (LONG)seed_pt.y + i;
					dc.SetPixel(pt, RGB(0, 255, 0));
				}
			}
		}
	}

	//  3D seed 功能
	if (m_3Dseed)
	{
		CPoint pt;
			
		// 於2D影像顯示在3D區域成長結果
		if (get_regionGrow)
		{
			for (j = 0; j < 512; j++)
			{
				for (i = 0; i < 512; i++)
				{
					if (judge[DisplaySlice][j * Col + i] == 1)
					{
						pt.x = i;
						pt.y = j;

						dc.SetPixel(pt, RGB(255, 120, 190));
					}
					else if (judge[DisplaySlice][j * Col + i] == 2)
					{
						pt.x = i;
						pt.y = j;

						dc.SetPixel(pt, RGB(80, 80, 255));
					}
				}
			}
		}
		// 於2D影像顯示在3D點選的seed
		if (get_3Dseed)
		{
			if (slice == seed_img.z)
			{
				for (i = -1; i <= 1; i++)
				{
					for (j = -1; j <= 1; j++)
					{
						pt.x = (LONG)seed_img.x + j;
						pt.y = (LONG)seed_img.y + i;

						dc.SetPixel(pt, RGB(255, 0, 0));
					}
				}
			}
		}
	}

	// 看看每一張的頂點 ( 二次區域成長就靠它了 )
	//
	if (!vertex.empty())
	{
		std::map<int, std::vector<std::pair<int, int>>>::iterator iter;
		iter = vertex.find(slice);
		CPoint pt;

		for (int k = 0; k < iter->second.size(); k++)
		{
			for (i = -3; i <= 3; i++)
			{
				for (j = -3; j <= 3; j++)
				{
					pt.x = (LONG)iter->second.at(k).first + i;
					pt.y = (LONG)iter->second.at(k).second + j;

					dc.SetPixel(pt, RGB(255, 30, 30));
				}
			}
		}
	}

	// 寫字 (slice)
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void C3DProcess::pointToVector(int x, int y, int width, int height, float vec[3])
{
	// DO : project x, y onto a hemi-sphere centered within width, height
	//
	float d, a;

	vec[0] = (float)((2.0 * x - width) / width);
	vec[1] = (float)((height - 2.0 * y) / height);
	vec[1] = (float)((mode == ControlModes::ControlObject) ? vec[1] : -vec[1]);		//v1
	
	d = sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
	
	vec[2] = (float)(cos((M_PI / 2.0) * ((d < 1.0) ? d : 1.0)));
	vec[2] = (float)((mode == ControlModes::ControlObject) ? vec[2] : -vec[2]);		//v2, just for moving plane normal
	
	a = (float)(1.0 / sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]));
	
	vec[0] *= a;
	vec[1] *= a;
	vec[2] *= a;
}

void C3DProcess::ActStart(UINT nFlags, int x, int y)
{
	// DO : 按下滑鼠後開始的動作
	//
	if (nFlags == MK_LBUTTON)
	{
		Act_Rotate = true;
		LR_Button = true;

		pointToVector(x, y, m_3D_rect.right - m_3D_rect.left, m_3D_rect.bottom - m_3D_rect.top, lastPos);
	} 
	else if (nFlags == MK_RBUTTON)
	{
		Act_Translate = true;
		LR_Button = false;

		transY = (float)y;
	}
}

void C3DProcess::ActStop(UINT nFlags, int x, int y)
{
	// DO : 放掉滑鼠後的動作
	// 另外用一個bool的變數來判斷滑鼠按鍵是因為放掉滑鼠時，nFlags都是0。
	//
	if (LR_Button == true)
	{
		Act_Rotate = false;

		if (mode == ControlModes::ControlObject)
			obj_angle = 0.0;
	}
	else if (LR_Button == false)
	{
		Act_Translate = false;
	}
}

void C3DProcess::ActTracking(int x, int y)
{
	// DO : 按住滑鼠時所執行的動作
	//
	if (Act_Rotate)
	{
		float curPos[3], dx, dy, dz;
		
		pointToVector(x, y, m_3D_rect.right - m_3D_rect.left, m_3D_rect.bottom - m_3D_rect.top, curPos);

		dx = curPos[0] - lastPos[0];
		dy = curPos[1] - lastPos[1];
		dz = curPos[2] - lastPos[2];

		if (mode == ControlModes::ControlObject)
		{
			obj_angle = (float)(90.0 * sqrt(dx * dx + dy * dy + dz * dz));

			obj_axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1];
			obj_axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
			obj_axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];
		}
		else if (mode == ControlModes::ControlPlane)
		{
			pln_angle = (float)(90.0 * sqrt(dx * dx + dy * dy + dz * dz));

			pln_axis[0] = lastPos[1] * curPos[2] - lastPos[2] * curPos[1];
			pln_axis[1] = lastPos[2] * curPos[0] - lastPos[0] * curPos[2];
			pln_axis[2] = lastPos[0] * curPos[1] - lastPos[1] * curPos[0];
		}

		lastPos[0] = curPos[0];
		lastPos[1] = curPos[1];
		lastPos[2] = curPos[2];
	}
	if (Act_Translate)
	{
		if (mode == ControlModes::ControlObject)
		{
			viewDistance += 0.01F * (y - transY);
			transY = (float)y;
		}
		else if (mode == ControlModes::ControlPlane)
		{
			user_Plane[3] -= 0.01F * (y - transY);
			transY = (float)y;
		}
	}
	UpdateWindow();
}

void C3DProcess::InvertMat(float (&m)[16])
{
	// DO : Invert Matrix
	//
	float temp;

	temp = m[1];
	m[1] = m[4];
	m[4] = temp;

	temp = m[2];
	m[2] = m[8];
	m[8] = temp;

	temp = m[6];
	m[6] = m[9];
	m[9] = temp;

	m[12] = -m[12];
	m[13] = -m[13];
	m[14] = -m[14];
}

C3DProcess::Seed_s C3DProcess::coordiConvert(Seed_d& pt)
{
	// openGL coordinate -> data array
	//
	// 假設立方體長.寬.高都是 512 (以矩陣來說)，頂點位置都是從 -1 到 1 (以GL空間來說)，
	// 則換算種子點的某軸位置時，先計算矩陣與GL空間的比例 ，再計算矩陣(或切片)一半的數量，
	// 最後兩者相乘換算後除以 2 ，就是原始影像兩端某一端在 openGL 空間的軸位置，
	// 但因為影像還有做縮放，所以除以縮放比例後獲得的結果，才是頂端真正的軸位置，
	// 接著，因為兩端位置是對稱關係，所以最後利用某一端端位置、另一端位置、矩陣數、
	// 以及現在種子點的軸位置去做三角比例換算，獲得的結果即為種子點的切片位置。
	
	Seed_s temp;
	double tmp_x, tmp_y, tmp_z;

	x_index = (2.0F / 511.0F) * (COL / 2.0F) / 2;
	x_index = x_index / scale_y;
	tmp_x = ((pt.x + 1) - (-x_index + 1)) * (COL / ((x_index + 1) - (-x_index + 1)));

	if (tmp_x - (int)tmp_x == 0)
		temp.x = (short)tmp_x - 1;
	else
		temp.x = (short)tmp_x;

	//-------------------------------------------------------------------------------//

	y_index = (2.0F / 511.0F) * (ROW / 2.0F) / 2;
	y_index = y_index / scale_z;
	tmp_y = ((pt.y + 1) - (-y_index + 1)) * (ROW / ((y_index + 1) - (-y_index + 1)));

	if (tmp_y - (int)tmp_y == 0)
		temp.y = (short)tmp_y - 1;
	else
		temp.y = (short)tmp_y;
	
	//-------------------------------------------------------------------------------//

	z_index = (2.0F / 511.0F) * (Total_Slice / 2.0F) / 2;
	z_index = z_index / scale_x;
	
	tmp_z = ((pt.z + 1) - (-z_index + 1)) * (Total_Slice / ((z_index + 1) - (-z_index + 1)));

	if (tmp_z < 1)
		tmp_z = (double)1;
	else if (tmp_z > Total_Slice)
		tmp_z = (double)Total_Slice;

	tmp_z -= 1;
	temp.z = (short)tmp_z;
	
	return temp;
}

void* C3DProcess::new2Dmatrix(int h, int w, int size)
{
	// 動態配置二維矩陣
	//
	int i;
	void** p;

	p = (void**) new char[h * sizeof(void*) + h * w * size];
	for (i = 0; i < h; i++)
		p[i] = ((char*)(p + h)) + i * w * size;

	return p;
}

void* C3DProcess::new3Dmatrix(int h, int w, int l, int size)
{
	int i, j;
	void*** p;

	p = (void***)new char[h * sizeof(void**) + h * w * sizeof(void*) + h * w * l * size];

	for (j = 0; j < h; j++)
	{
		p[j] = ((void**)(p + h)) + j * w;
		for (i = 0; i < w; i++)
		{
			p[j][i] = ((char*)(p + h + h * w + j * w * l)) + i * l * size;
		}
	}
	return p;
}

void* C3DProcess::new4Dmatrix(int h, int w, int l, int v, int size)
{
	int i, j, k;
	void**** p;

	p = (void****)new char[h * sizeof(void***) + h * w * sizeof(void**) + h * w * l * sizeof(void*) + h * w * l * v * size];

	for (k = 0; k < h; k++)
	{
		p[k] = ((void***)(p + h)) + k * w;
		for (j = 0; j < w; j++)
		{
			p[k][j] = ((void**)(p + h + h * w + k * w * l)) + j * l;
			for (i = 0; i < l; i++)
			{
				p[k][j][i] = ((char*)(p + h + h * w + h * w * l + k * w * l * v + j * l * v)) + i * v * size;
			}
		}
	}
	return p;
}

void C3DProcess::RG_3D_GlobalAvgConnected(short** src, RG_factor& factor)
{
	//	DO : 3D 區域成長 
	//	利用「當前已成長的全域平均值」來界定成長標準，並用「目前的像素強度」來判斷
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int range = (factor.s_kernel - 1) / 2;	// 判斷範圍
	register int i, j, k;
	unsigned int cnt = 1;							// 計數成長的pixel數量
	short n_pixel = 0;
	
	double avg;
	double up_limit;
	double down_limit;
	double threshold = factor.threshold;
	
	Seed_s temp;								// 當前 判斷的周圍seed
	Seed_s current;								// 當前 判斷的中心seed
	Seed_s seed = factor.seed;					// 初始seed
	queue<double> avg_que;						// 暫存某點成長判斷完，當下已成長區域的整體avg
	queue<Seed_s> sed_que;						// 暫存成長判斷為種子點的像素位置

	avg = m_pDoc->m_img[seed.z][(seed.y) * col + (seed.x)];
	src[seed.z][(seed.y) * col + (seed.x)] = 1;
	sed_que.push(seed);
	avg_que.push(avg);

	while (!sed_que.empty())
	{
		avg = avg_que.front();
		current = sed_que.front();
		up_limit = avg + threshold;
		down_limit = avg - threshold;

		// 重新界定成長標準上下限
 		if (up_limit > 255)
		{
			up_limit = 255;
			down_limit = 255 - 2 * threshold;
		}
		if (down_limit < 0)
		{
			down_limit = 0;
			up_limit = 0 + 2 * threshold;
		}

		// 判斷周圍區域是否符合成長標準
		// 並同時計算「已成長區域」的總平均
		for (k = -range; k <= range; k++)
		{
			for (j = -range; j <= range; j++)
			{
				for (i = -range; i <= range; i++)
				{
					if ((current.x + i) < (col)			&& (current.x + i) >= 0 &&
						(current.y + j) < (row)			&& (current.y + j) >= 0 &&
						(current.z + k) < (totalSlice)	&& (current.z + k) >= 0)
					{
						if (src[current.z + k][(current.y + j) * col + (current.x + i)] == 0)
						{
							n_pixel = m_pDoc->m_img[current.z + k][(current.y + j) * col + (current.x + i)];

							if ((n_pixel <= up_limit) && (n_pixel >= down_limit))
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								sed_que.push(temp);

								src[current.z + k][(current.y + j) * col + (current.x + i)] = 1;
								
								avg = (avg * cnt + n_pixel) / (cnt + 1);
								avg_que.push(avg);
								cnt += 1;
							}
							else
								src[current.z + k][(current.y + j) * col + (current.x + i)] = -1;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

}

void C3DProcess::RG_3D_LocalAvgConnected(short** src, RG_factor& factor)
{
	//	DO : 3D 區域成長
	//	利用「當前已成長的全域平均值」來界定成長標準，並用「目前像素鄰近區域的平均值」來判斷
	//  (s_ : seed ; n_ : nerighbor)
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const int n_range = (factor.n_kernel - 1) / 2;
	register int si, sj, sk;
	register int ni, nj, nk;
	unsigned int s_cnt = 1;
	unsigned int n_cnt = 0;
	int n_pixel = 0;

	double	s_avg;
	double	n_avg;
	double	up_limit;
	double	down_limit;
	double	threshold = factor.threshold;

	Seed_s	temp;
	Seed_s	s_current;
	Seed_s	n_current;
	Seed_s	seed = factor.seed;
	queue<Seed_s>	sed_que;
	queue<double>	avg_que;
	
	s_avg = m_pDoc->m_img[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = 1;
	avg_que.push(s_avg);
	sed_que.push(seed);

	while (!sed_que.empty())
	{
		s_avg = avg_que.front();
		s_current = sed_que.front();
		up_limit = s_avg + threshold;
		down_limit = s_avg - threshold;

		// 重新界定成長標準
		if (up_limit > 255)
		{
			up_limit = 255;
			down_limit = 255 - 2 * threshold;
		}
		if (down_limit < 0)
		{
			down_limit = 0;
			up_limit = 0 + 2 * threshold;
		}

		// 判斷周圍區域
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if ((s_current.x + si) < col		&& (s_current.x + si) >= 0 &&
						(s_current.y + sj) < row		&& (s_current.y + sj) >= 0 &&
						(s_current.z + sk) < totalSlice && (s_current.z + sk) >= 0	)
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] == 0)
						{
							n_current.x = s_current.x + si;
							n_current.y = s_current.y + sj;
							n_current.z = s_current.z + sk;
							n_pixel = 0;	n_cnt = 0;

							// 計算周圍區域像素強度的平均值
							for (nk = -n_range; nk <= n_range; nk++)
							{
								for (nj = -n_range; nj <= n_range; nj++)
								{
									for (ni = -n_range; ni <= n_range; ni++)
									{
										if ((n_current.x + ni) < col && (n_current.x + ni) >= 0 &&
											(n_current.y + nj) < row && (n_current.y + nj) >= 0 &&
											(n_current.z + nk) < totalSlice && (n_current.z + nk) >= 0)
										{
											n_pixel +=
												m_pDoc->m_img[n_current.z + nk][(n_current.y + nj) * col + (n_current.x + ni)];
											n_cnt += 1;
										}
									}
								}
							}
							n_avg = (double)n_pixel / n_cnt;

							// 判斷是否符合成長標準
							if ((n_avg <= up_limit) && (n_avg >= down_limit))
							{
								sed_que.push(n_current);
								src[n_current.z][n_current.y * col + n_current.x] = 1;
								n_pixel = m_pDoc->m_img[n_current.z][n_current.y * col + n_current.x];
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
							else
								src[n_current.z][n_current.y * col + n_current.x] = -1;
						}
					}
				}
			}
		}
		sed_que.pop();
		avg_que.pop();
	}

}

void C3DProcess::RG_3D_ConfidenceConnected(short** src, RG_factor& factor)
{
	// DO : 3D 區域成長
	// 利用當前區域的「平均值」與「標準差」界定成長標準，並以「像素強度」來判斷
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	unsigned long long  n_pixel = 0, s_pixel = 0;
	unsigned long long	s_cnt = 0;
	register int si, sj, sk;

	double	s_avg;
	double	up_limit, down_limit;
	double	threshold = factor.threshold;
	double	coefficient = factor.coefficient;
	
	Seed_s	n_site;
	Seed_s	s_current;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;
	std::vector<int> pix;

	auto outOfRange = [=](int px, int py, int pz)
	{	// 判斷有無超出影像邊界
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};
	
	s_avg = imgPro[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = 1;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;
	
	while (!sed_que.empty())
	{
		s_avg = avg_que.front();
		s_current = sed_que.front();

		// 計算 總合 與 平均
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if (!outOfRange(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						sum +=
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						cnt += 1;
						pix.push_back(imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)]);
					}
				}
			}
		}
		n_avg = sum / cnt;

		// 計算 標準差
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if (!outOfRange(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						n_pixel =
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);

		// 先判斷該種子點周圍的平均與標準差是否符合標準
		if (n_sd > 15 || abs(n_avg - s_avg) > threshold)
		{
			judge[s_current.z][s_current.y * col + s_current.x] = -1;
			sed_que.pop();
			avg_que.pop();
			s_cnt -= 1;
			continue;
		}
		
		// 制定、修正成長標準的上下限
		up_limit = n_avg + (coefficient * n_sd);
		down_limit = n_avg - (coefficient * n_sd);
		
		// 判斷是否符合成長標準
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if (!outOfRange(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] == 0)
						{
							n_pixel =
								imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];

							if (n_pixel <= up_limit && n_pixel >= down_limit)
							{
								n_site.x = s_current.x + si;
								n_site.y = s_current.y + sj;
								n_site.z = s_current.z + sk;
								sed_que.push(n_site);

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = 1;
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
							else
								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = -1;
						}
					}
				}
			}
		}
		sed_que.pop();
		avg_que.pop();
		pix.clear();
	}

}

void C3DProcess::RG2_3D_ConfidenceConnected(short** src, RG_factor& factor)
{
	// DO : 3D - 2222222222222222222222222222222222 次區域成長
	// 利用當前區域的「平均值」與「標準差」界定成長標準，並以「像素強度」來判斷
	//
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	unsigned int s_cnt = 0, n_pixel = 0;
	register int si, sj, sk;
	
	double	s_avg = 0;
	double	up_limit, down_limit;
	double	threshold = factor.threshold;
	double	coefficient = factor.coefficient;

	Seed_s	n_site;
	Seed_s	s_current;
	Seed_s	seed = factor.seed;
	queue<Seed_s> sed_que;
	queue<double> avg_que;

	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	auto lineFunc_1 = [=](int px, int py, int pz)	// left line
	{
		float value = line[pz][0].first * px + line[pz][0].second - py;
		if (value <= 0)	return true;				// 在left line的左邊(要倒著看
		else return false;
	};
	auto lineFunc_2 = [=](int px, int py, int pz)	// right line
	{
		float value = line[pz][1].first * px + line[pz][1].second - py;
		if (value <= 0)	return true;				// 在right line的右邊(要倒著看
		else return false;
	};
	auto outOfRange = [=](int px, int py, int pz)	// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};
	
	// 先把第一次區域成長的種子點都加進來
	// 原本判定不要的先歸零
	for (sj = 0; sj < totalSlice; ++sj)
	{
		for (si = 0; si < totalXY; ++si)
		{
			if (src[sj][si] == 1)
			{
				n_site.x = si % col;
				n_site.y = si / col;
				n_site.z = sj;

				n_pixel = img[sj][si];
				s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
				s_cnt += 1;

				avg_que.push(s_avg);
				sed_que.push(n_site);
			}
			else if (src[sj][si] == -1)
				src[sj][si] = 0;
		}
	}

	// 做第二次區域成長
	while (!sed_que.empty())
	{
		s_avg = avg_que.front();
		s_current = sed_que.front();

		// 制定、修正成長標準上下限
		up_limit = s_avg + threshold;
		down_limit = s_avg - threshold;

		// 重新界定成長標準上下限
		if (up_limit > 255 || down_limit < 0)
		{
			if (down_limit > 0)
			{
				up_limit = 255;
				down_limit = 255 - 2 * threshold;
			}
			else
			{
				down_limit = 0;
				up_limit = 0 + 2 * threshold;
			}
		}

		// 判斷是否符合成長標準
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfRange(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] == 0)
						{
							n_pixel =
								imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];

							if (n_pixel <= up_limit && n_pixel >= down_limit &&
								lineFunc_1(s_current.x + si, s_current.y + sj, s_current.z + sk) &&
								lineFunc_2(s_current.x + si, s_current.y + sj, s_current.z + sk)
								)
							{
								n_site.x = s_current.x + si;
								n_site.y = s_current.y + sj;
								n_site.z = s_current.z + sk;
								sed_que.push(n_site);

								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = 2;
							}
							else
								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = -1;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}
	
}

void C3DProcess::RG_3D_Link(short** src, RG_factor& factor)
{
	//	DO : 3D 區域成長
	//	將與種子點連接的像素點「連接」起來(確認最終分割區域與體積)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int totalSlice = Total_Slice;
	const int kernel = factor.n_kernel;
	const int range = (kernel - 1) / 2;			// 判斷範圍
	register int i, j, k;

	// src : 原始以及將要被更改的矩陣
	// temp : 暫存原始狀態的矩陣(不做更動)
	short** src_temp = New2Dmatrix(totalSlice, total_xy, short);

	Seed_s temp;								// 當前 判斷的周圍seed
	Seed_s current;								// 當前 判斷的中心seed
	Seed_s seed = factor.seed;					// 初始seed
	queue<Seed_s> sd_que;						// 暫存成長判斷為種子點的像素位置

	// Deep copy (目前先以這樣的方式處理QQ)
	//
	for (j = 0; j < totalSlice; j++)
	{
		for (i = 0; i < total_xy; i++)
		{
			src_temp[j][i] = src[j][i];
			src[j][i] = 0;
		}
	}

	src[seed.z][(seed.y) * col + (seed.x)] = 1;
	sd_que.push(seed);

	while (!sd_que.empty())
	{
		current = sd_que.front();
		for (k = -range; k <= range; k++)
		{
			for (j = -range; j <= range; j++)
			{
				for (i = -range; i <= range; i++)
				{
					if ((current.x + i) < (col) && (current.x + i) >= 0 &&
						(current.y + j) < (row) && (current.y + j) >= 0 &&
						(current.z + k) < (totalSlice) && (current.z + k) >= 0)
					{
						if (src[current.z + k][(current.y + j) * col + (current.x + i)] != 1 &&
							src_temp[current.z + k][(current.y + j) * col + (current.x + i)] == 1)
						{
							temp.x = current.x + i;
							temp.y = current.y + j;
							temp.z = current.z + k;
							sd_que.push(temp);

							src[current.z + k][(current.y + j) * col + (current.x + i)] = 1;
						}
					}
				}
			}
		}
		sd_que.pop();
	}
	delete src_temp;
}

void C3DProcess::Erosion_3D(short** src, short element)
{
	// DO : 3D Erosion (侵蝕 - 形態學處理)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int total_z = Total_Slice;
	register int si, sj, sk;
	register int i, j, k;

	// src : 原始以及將要被更改的矩陣
	// temp : 暫存原始狀態的矩陣(不做更動)
	short** temp = New2Dmatrix(total_z, total_xy, short);
	
	// Deep copy (目前先以這樣的方式處理QQ)
	//
	for (j = 0; j < total_z; j++)
	{
		for (i = 0; i < total_xy; i++)
		{
			temp[j][i] = src[j][i];
		}
	}
	// Erosion
	//
	// 計算周圍符合條件的pixel的數量
	// 若數量符合條件，則為 1 (不侵蝕)
	if (element == 0)
	{
		int amount = 13;					// 數量目標
		int range = (3 - 1) / 2;			// 判定區域的範圍
		int n = 0;							// 計算周圍符合條件的pixel數量
		
		// 遍歷所有為 1 的 pixel
		for (sk = range; sk < total_z - range; sk++)
		{
			for (sj = range; sj < row - range; sj++)
			{
				for (si = range; si < col - range; si++)
				{
					n = 0;
					// 判斷周圍區域也為 1 的數量
					if (temp[sk][sj * col + si] == 1)
					{
						for (k = -range; k <= range; k++)
						{
							for (j = -range; j <= range; j++)
							{
								for (i = -range; i <= range; i++)
								{
									if (temp[sk + k][(sj + j) * col + (si + i)] == 1)
									{
										n += 1;
									}
								}
							}
						}
						if (n >= amount)
						{
							src[sk][sj * col + si] = 1;
						}
						else
						{
							src[sk][sj * col + si] = 0;
						}
					}
				}
			}
		}
	}
	// 6 鄰域
	else if (element == 6)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						if ((temp[(k + -1)][(j + 0) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + -1) * col + (i + 0)] == 1) &&
							(temp[(k + 0)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + 0)][(j + 1) * col + (i + 0)] == 1) && (temp[(k + 1)][(j + 0) * col + (i + 0)] == 1)
							)
						{
							src[k][j * col + i] = 1;
						}
						else
						{
							src[k][j * col + i] = 0;
						}
					}
				}
			}
		}
	}
	// 18 鄰域
	else if (element == 18)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						if ((temp[(k + -1)][(j + -1) * col + (i + 0)] == 1) && (temp[(k + -1)][(j + 0) * col + (i + -1)] == 1) &&
							(temp[(k + -1)][(j + 0) * col + (i + 0)] == 1) && (temp[(k + -1)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + -1)][(j + 1) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + -1) * col + (i + -1)] == 1) &&

							(temp[(k + 0)][(j + -1) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + -1) * col + (i + 1)] == 1) &&
							(temp[(k + 0)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + 0)][(j + 1) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + 1) * col + (i + 0)] == 1) &&

							(temp[(k + 0)][(j + 1) * col + (i + 1)] == 1) && (temp[(k + 1)][(j + -1) * col + (i + 0)] == 1) &&
							(temp[(k + 1)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + 1)][(j + 0) * col + (i + 0)] == 1) &&
							(temp[(k + 1)][(j + 0) * col + (i + 1)] == 1) && (temp[(k + 1)][(j + 1) * col + (i + 0)] == 1)
							)
						{
							src[k][j * col + i] = 1;
						}
						else
						{
							src[k][j * col + i] = 0;
						}
					}
				}
			}
		}
	}
	// 26 鄰域
	else if (element == 26)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						if ((temp[(k + -1)][(j + -1) * col + (i + -1)] == 1) && (temp[(k + -1)][(j + -1) * col + (i + 0)] == 1) && (temp[(k + -1)][(j + -1) * col + (i + 1)] == 1) &&
							(temp[(k + -1)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + -1)][(j + 0) * col + (i + 0)] == 1) && (temp[(k + -1)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + -1)][(j + 1) * col + (i + -1)] == 1) && (temp[(k + -1)][(j + 1) * col + (i + 0)] == 1) && (temp[(k + -1)][(j + 1) * col + (i + 1)] == 1) &&

							(temp[(k + 0)][(j + -1) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + -1) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + -1) * col + (i + 1)] == 1) &&
							(temp[(k + 0)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + 0) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + 0)][(j + 1) * col + (i + -1)] == 1) && (temp[(k + 0)][(j + 1) * col + (i + 0)] == 1) && (temp[(k + 0)][(j + 1) * col + (i + 1)] == 1) &&

							(temp[(k + 1)][(j + -1) * col + (i + -1)] == 1) && (temp[(k + 1)][(j + -1) * col + (i + 0)] == 1) && (temp[(k + 1)][(j + -1) * col + (i + 1)] == 1) &&
							(temp[(k + 1)][(j + 0) * col + (i + -1)] == 1) && (temp[(k + 1)][(j + 0) * col + (i + 0)] == 1) && (temp[(k + 1)][(j + 0) * col + (i + 1)] == 1) &&
							(temp[(k + 1)][(j + 1) * col + (i + -1)] == 1) && (temp[(k + 1)][(j + 1) * col + (i + 0)] == 1) && (temp[(k + 1)][(j + 1) * col + (i + 1)] == 1)
							)
						{
							src[k][j * col + i] = 1;
						}
						else
						{
							src[k][j * col + i] = 0;
						}
					}
				}
			}
		}
	}
	delete temp;
}

void C3DProcess::Dilation_3D(short** src, short element)
{
	// DO : 3D Dilation (膨脹 - 形態學處理)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int total_z = Total_Slice;
	register int i, j, k;

	// src : 原始以及將要被更改的矩陣
	// temp : 暫存原始狀態的矩陣(不做更動)
	short** temp = New2Dmatrix(total_z, total_xy, short);

	// Deep copy (目前先以這樣的方式處理QQ)
	//
	for (j = 0; j < total_z; j++)
	{
		for (i = 0; i < total_xy; i++)
		{
			temp[j][i] = src[j][i];
		}
	}
	// Dilation
	//
	// 6 鄰域
	if (element == 6)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						src[(k + -1)][(j + 0) * col + (i + 0)] = 1;
						src[(k + 0)][(j + -1) * col + (i + 0)] = 1;
						src[(k + 0)][(j + 0) * col + (i + -1)] = 1;
						src[(k + 0)][(j + 0) * col + (i + 1)] = 1;
						src[(k + 0)][(j + 1) * col + (i + 0)] = 1;
						src[(k + 1)][(j + 0) * col + (i + 0)] = 1;
					}
				}
			}
		}
	}
	// 18 鄰域
	else if (element == 18)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						src[(k + -1)][(j + -1) * col + (i + 0)] = 1;
						src[(k + -1)][(j + 0) * col + (i + -1)] = 1;
						src[(k + -1)][(j + 0) * col + (i + 0)] = 1;
						src[(k + -1)][(j + 0) * col + (i + 1)] = 1;
						src[(k + -1)][(j + 1) * col + (i + 0)] = 1;
						
						src[(k + 0)][(j + -1) * col + (i + -1)] = 1;
						src[(k + 0)][(j + -1) * col + (i + 0)] = 1;
						src[(k + 0)][(j + -1) * col + (i + 1)] = 1;
						src[(k + 0)][(j + 0) * col + (i + -1)] = 1;
						src[(k + 0)][(j + 0) * col + (i + 1)] = 1;
						src[(k + 0)][(j + 1) * col + (i + -1)] = 1;
						src[(k + 0)][(j + 1) * col + (i + 0)] = 1;
						src[(k + 0)][(j + 1) * col + (i + 1)] = 1;
						
						src[(k + 1)][(j + -1) * col + (i + 0)] = 1;
						src[(k + 1)][(j + 0) * col + (i + -1)] = 1;
						src[(k + 1)][(j + 0) * col + (i + 0)] = 1;
						src[(k + 1)][(j + 0) * col + (i + 1)] = 1;
						src[(k + 1)][(j + 1) * col + (i + 0)] = 1;
					}
				}
			}
		}
	}
	// 26 鄰域
	else if (element == 26)
	{
		for (k = 1; k < total_z - 1; k++)
		{
			for (j = 1; j < row - 1; j++)
			{
				for (i = 1; i < col - 1; i++)
				{
					if (temp[k][j * col + i] == 1)
					{
						src[(k + -1)][(j + -1) * col + (i + -1)] = 1;
						src[(k + -1)][(j + -1) * col + (i + 0)] = 1;
						src[(k + -1)][(j + -1) * col + (i + 1)] = 1;
						src[(k + -1)][(j + 0) * col + (i + -1)] = 1;
						src[(k + -1)][(j + 0) * col + (i + 0)] = 1;
						src[(k + -1)][(j + 0) * col + (i + 1)] = 1;
						src[(k + -1)][(j + 1) * col + (i + -1)] = 1;
						src[(k + -1)][(j + 1) * col + (i + 0)] = 1;
						src[(k + -1)][(j + 1) * col + (i + 1)] = 1;
						
						src[(k + 0)][(j + -1) * col + (i + -1)] = 1;
						src[(k + 0)][(j + -1) * col + (i + 0)] = 1;
						src[(k + 0)][(j + -1) * col + (i + 1)] = 1;
						src[(k + 0)][(j + 0) * col + (i + -1)] = 1;
						//src[(k + 0)][(j + 0) * col + (i + 0)] = 1;
						src[(k + 0)][(j + 0) * col + (i + 1)] = 1;
						src[(k + 0)][(j + 1) * col + (i + -1)] = 1;
						src[(k + 0)][(j + 1) * col + (i + 0)] = 1;
						src[(k + 0)][(j + 1) * col + (i + 1)] = 1;
						
						src[(k + 1)][(j + -1) * col + (i + -1)] = 1;
						src[(k + 1)][(j + -1) * col + (i + 0)] = 1;
						src[(k + 1)][(j + -1) * col + (i + 1)] = 1;
						src[(k + 1)][(j + 0) * col + (i + -1)] = 1;
						src[(k + 1)][(j + 0) * col + (i + 0)] = 1;
						src[(k + 1)][(j + 0) * col + (i + 1)] = 1;
						src[(k + 1)][(j + 1) * col + (i + -1)] = 1;
						src[(k + 1)][(j + 1) * col + (i + 0)] = 1;
						src[(k + 1)][(j + 1) * col + (i + 1)] = 1;
					}
				}
			}
		}
	}
	delete temp;
}

double C3DProcess::Calculate_Volume(short** src)
{
	const int totalXY = COL * ROW;
	const int totalSlice = Total_Slice;
	register int i, j;
	unsigned long long n = 0;							// 計數成長的pixel數量
	for (j = 0; j < totalSlice; j++)
	{
		for (i = 0; i < totalXY; i++)
		{
			if (src[j][i] > 0)
				n += 1;
		}
	}
	double volume = 0L;		// 單位 (cm3)
	volume = (n * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z) / 1000;	
	return volume;
}



