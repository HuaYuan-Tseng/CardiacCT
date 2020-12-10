
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
	, m_2Dseed(FALSE)
	, m_2Dverify(FALSE)
	, m_3Dseed(FALSE)
	, m_spine(FALSE)
	, m_sternum(FALSE)
	, m_object(TRUE)
	, m_plane(FALSE)
	, m_disp_org(TRUE)
	, m_disp_pro0(FALSE)
	, m_complete(TRUE)
	, m_thresholdHU(FALSE)
	, m_thresholdPixel(FALSE)
	, m_spine_verify(FALSE)
	, m_sternum_verify(FALSE)
	, m_HU_up_threshold(_T("3072"))
	, m_HU_down_threshold(_T("200"))
	, m_pixel_up_threshold(_T("255"))
	, m_pixel_down_threshold(_T("150"))
	, m_intensity(_T("0.8125"))
	, m_density(_T("0.0"))
	, m_slices(_T("512"))
	, m_pos_1(_T("0"))
	, m_pos_2(_T("0"))
	, m_pos_3(_T("0"))
	, m_pos_4(_T("0"))
	, m_pos_5(_T("0"))
	, m_pos_6(_T("0"))
	, m_pos_7(_T("0"))
	, m_pos_8(_T("0"))
	, m_result(_T("0.0"))
	, m_result_2(_T("0.0"))
	, m_pix_th(_T("50.0"))
	, m_SDth(_T("20.0"))
	, m_SDco(_T("1.0"))
	, m_sKernel(_T("3"))
	, m_nKernel(_T("3"))
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
	get_3Dimage = false;
	get_regionGrow = false;
	get_mid_fix = false;
	get_sternum = false;
	get_spine = false;
	
	draw_pt_cnt = 0;
	draw_pt_total = 2;
	verify_reference_slice = 0;
	get_verify_reference = false;

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
	spine_volume = 0.0;
	sternum_volume = 0.0;
	HU_up_threshold = atoi(m_HU_up_threshold);
	HU_down_threshold = atoi(m_HU_down_threshold);
	pixel_up_threshold = atoi(m_pixel_up_threshold);
	pixel_down_threshold = atoi(m_pixel_down_threshold);

	RG_term.s_kernel = atoi(m_sKernel);
	RG_term.n_kernel = atoi(m_nKernel);
	RG_term.pix_thresh = atof(m_pix_th);
	RG_term.sd_thresh = atof(m_SDth);
	RG_term.sd_coeffi = atof(m_SDco);

	seed_pt = { 0, 0, 0 };
	seed_img = { 0, 0, 0 };
	seed_gl = { 0.0L, 0.0L, 0.0L };
	mid_fix_pt = std::make_pair(0, 0);
	
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
	
	if (m_pixel_down_threshold.IsEmpty() != true)
		m_pixel_down_threshold.Empty();
	if (m_pixel_up_threshold.IsEmpty() != true)
		m_pixel_up_threshold.Empty();
	if (m_HU_down_threshold.IsEmpty() != true)
		m_HU_down_threshold.Empty();
	if (m_HU_up_threshold.IsEmpty() != true)
		m_HU_up_threshold.Empty();
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
	if (m_sKernel.IsEmpty() != true)
		m_sKernel.Empty();
	if (m_nKernel.IsEmpty() != true)
		m_nKernel.Empty();
	if (m_pix_th.IsEmpty() != true)
		m_pix_th.Empty();
	if (m_SDth.IsEmpty() != true)
		m_SDth.Empty();
	if (m_SDco.IsEmpty() != true)
		m_SDco.Empty();

	if (spine_vertex.empty() != true)
	{
		for (auto& n : spine_vertex)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		spine_vertex.swap(empty_map);
		spine_vertex.clear();
	}
	if (spine_line.empty() != true)
	{
		for (auto& n : spine_line)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<float, float>>> empty_map;
		spine_line.swap(empty_map);
		spine_line.clear();
	}
	if (spine_edge.empty() != true)
	{
		for (auto& n : spine_edge)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<int>> empty_map;
		spine_edge.swap(empty_map);
		spine_edge.clear();
	}
	if (sternum_vertex.empty() != true)
	{
		for (auto& n : sternum_vertex)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		sternum_vertex.swap(empty_map);
		sternum_vertex.clear();
	}
	if (sternum_line.empty() != true)
	{
		for (auto& n : sternum_line)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<float, float>>> empty_map;
		sternum_line.swap(empty_map);
		sternum_line.clear();
	}
	if (sternum_edge.empty() != true)
	{
		for (auto& n : sternum_edge)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<int>> empty_map;
		sternum_edge.swap(empty_map);
		sternum_edge.clear();
	}
	if (draw_spine_pt.empty() != true)
	{
		for (auto& n : draw_spine_pt)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		draw_spine_pt.swap(empty_map);
		draw_spine_pt.clear();
	}
	if (draw_sternum_pt.empty() != true)
	{
		for (auto& n : draw_sternum_pt)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		draw_sternum_pt.swap(empty_map);
		draw_sternum_pt.clear();
	}
	if (draw_spine_line.empty() != true)
	{
		for (auto& n : draw_spine_line)
		{
			n.second.clear();
			std::set<std::pair<int, int>> empty_set;
			n.second.swap(empty_set);
		}
		std::map<int, std::set<std::pair<int, int>>> empty_map;
		draw_spine_line.swap(empty_map);
		draw_spine_line.clear();
	}
	if (draw_sternum_line.empty() != true)
	{
		for (auto& n : draw_sternum_line)
		{
			n.second.clear();
			std::set<std::pair<int, int>> empty_set;
			n.second.swap(empty_set);
		}
		std::map<int, std::set<std::pair<int, int>>> empty_map;
		draw_sternum_line.swap(empty_map);
		draw_sternum_line.clear();
	}
	if (spine_interpolate_slice.empty() != true)
	{
		spine_interpolate_slice.clear();
		spine_interpolate_slice.shrink_to_fit();
	}
	if (sternum_interpolate_slice.empty() != true)
	{
		sternum_interpolate_slice.clear();
		sternum_interpolate_slice.shrink_to_fit();
	}

	glDeleteTextures(ImageFrame, textureName);
}

void C3DProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_2D, m_ScrollBar);

	DDX_Check(pDX, IDC_CHECK_PLANE, m_plane);
	DDX_Check(pDX, IDC_CHECK_Object, m_object);
	DDX_Check(pDX, IDC_CHECK_3D_SEED, m_3Dseed);
	DDX_Check(pDX, IDC_CHECK_2D_SEED, m_2Dseed);
	DDX_Check(pDX, IDC_CHECK_SPINE, m_spine);
	DDX_Check(pDX, IDC_CHECK_STERNUM, m_sternum);
	DDX_Check(pDX, IDC_CHECK_DISP_ORG, m_disp_org);
	DDX_Check(pDX, IDC_CHECK_DISP_PRO0, m_disp_pro0);
	DDX_Check(pDX, IDC_CHECK_2D_VERIFY, m_2Dverify);
	DDX_Check(pDX, IDC_CHECK_SPINE_VERIFY, m_spine_verify);
	DDX_Check(pDX, IDC_CHECK_STERNUM_VERIFY, m_sternum_verify);
	DDX_Check(pDX, IDC_CHECK_COMPLETE, m_complete);
	DDX_Check(pDX, IDC_CHECK_HU_THRESHOLD, m_thresholdHU);
	DDX_Check(pDX, IDC_CHECK_PIXEL_THRESHOLD, m_thresholdPixel);

	DDX_Text(pDX, IDC_EDIT_SLICES, m_slices);
	DDX_Text(pDX, IDC_EDIT_HU_UP_THRESHOLD, m_HU_up_threshold);
	DDX_Text(pDX, IDC_EDIT_HU_DOWN_THRESHOLD, m_HU_down_threshold);
	DDX_Text(pDX, IDC_EDIT_PIXEL_UP_THRESHOLD, m_pixel_up_threshold);
	DDX_Text(pDX, IDC_EDIT_PIXEL_DOWN_THRESHOLD, m_pixel_down_threshold);

	DDV_MinMaxInt(pDX, atoi(m_slices), 1, 512);
	DDV_MinMaxShort(pDX, atoi(m_pixel_up_threshold), 0, 255);
	DDV_MinMaxShort(pDX, atoi(m_pixel_down_threshold), 0, 255);
	DDV_MinMaxShort(pDX, atoi(m_HU_up_threshold), HU_Min, HU_Max);
	DDV_MinMaxShort(pDX, atoi(m_HU_down_threshold), HU_Min, HU_Max);

	DDX_Text(pDX, IDC_EDIT_INTENSITY, m_intensity);
	DDX_Text(pDX, IDC_EDIT_DENSITY, m_density);
	DDX_Text(pDX, IDC_EDIT_POS1, m_pos_1);
	DDX_Text(pDX, IDC_EDIT_POS2, m_pos_2);
	DDX_Text(pDX, IDC_EDIT_POS3, m_pos_3);
	DDX_Text(pDX, IDC_EDIT_POS4, m_pos_4);
	DDX_Text(pDX, IDC_EDIT_POS5, m_pos_5);
	DDX_Text(pDX, IDC_EDIT_POS6, m_pos_6);
	DDX_Text(pDX, IDC_EDIT_POS7, m_pos_7);
	DDX_Text(pDX, IDC_EDIT_POS8, m_pos_8);
	DDX_Text(pDX, IDC_EDIT_RESULT, m_result);
	DDX_Text(pDX, IDC_EDIT_RESULT_2, m_result_2);
	DDX_Text(pDX, IDC_EDIT_S_KERNEL, m_sKernel);
	DDX_Text(pDX, IDC_EDIT_N_KERNEL, m_nKernel);
	DDX_Text(pDX, IDC_EDIT_PIX_TH, m_pix_th);
	DDX_Text(pDX, IDC_EDIT_SD_TH, m_SDth);
	DDX_Text(pDX, IDC_EDIT_SD_CO, m_SDco);
	
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

	ON_BN_CLICKED(ID_EXIT, &C3DProcess::OnBnClickedExit)
	ON_BN_CLICKED(IDC_CHECK_SPINE, &C3DProcess::OnBnClickedCheckSpine)
	ON_BN_CLICKED(IDC_CHECK_STERNUM, &C3DProcess::OnBnClickedCheckSternum)
	ON_BN_CLICKED(IDC_CHECK_2D_VERIFY, &C3DProcess::OnBnClickedCheck2dVerify)
	ON_BN_CLICKED(IDC_CHECK_2D_SEED, &C3DProcess::OnBnClickedCheck2dSeed)
	ON_BN_CLICKED(IDC_CHECK_3D_SEED, &C3DProcess::OnBnClickedCheck3dSeed)
	ON_BN_CLICKED(IDC_CHECK_PLANE, &C3DProcess::OnBnClickedCheckPlane)
	ON_BN_CLICKED(IDC_CHECK_Object, &C3DProcess::OnBnClickedCheckObject)
	ON_BN_CLICKED(IDC_CHECK_DISP_ORG, &C3DProcess::OnBnClickedCheckDispOrg)
	ON_BN_CLICKED(IDC_CHECK_DISP_PRO0, &C3DProcess::OnBnClickedCheckDispPro0)
	ON_BN_CLICKED(IDC_CHECK_COMPLETE, &C3DProcess::OnBnClickedCheckComplete)
	ON_BN_CLICKED(IDC_CHECK_HU_THRESHOLD, &C3DProcess::OnBnClickedCheckHuThreshold)
	ON_BN_CLICKED(IDC_CHECK_PIXEL_THRESHOLD, &C3DProcess::OnBnClickedCheckPixelThreshold)
	ON_BN_CLICKED(IDC_CHECK_SPINE_VERIFY, &C3DProcess::OnBnClickedCheckSpineVerify)
	ON_BN_CLICKED(IDC_CHECK_STERNUM_VERIFY, &C3DProcess::OnBnClickedCheckSternumVerify)
	
	ON_BN_CLICKED(IDC_BUTTON_MID_FIX, &C3DProcess::OnBnClickedButtonMidFix)
	ON_BN_CLICKED(IDC_BUTTON_PLANE_RESET, &C3DProcess::OnBnClickedButtonPlaneReset)
	ON_BN_CLICKED(IDC_BUTTON_SLICES_PLUS, &C3DProcess::OnBnClickedButtonSlicesPlus)
	ON_BN_CLICKED(IDC_BUTTON_SLICES_MINUS, &C3DProcess::OnBnClickedButtonSlicesMinus)
	ON_BN_CLICKED(IDC_BUTTON_DENSITY_PLUS, &C3DProcess::OnBnClickedButtonDensityPlus)
	ON_BN_CLICKED(IDC_BUTTON_DENSITY_MINUS, &C3DProcess::OnBnClickedButtonDensityMinus)
	ON_BN_CLICKED(IDC_BUTTON_INTENSITY_PLUS, &C3DProcess::OnBnClickedButtonIntensityPlus)
	ON_BN_CLICKED(IDC_BUTTON_INTENSITY_MINUS, &C3DProcess::OnBnClickedButtonIntensityMinus)
	ON_BN_CLICKED(IDC_BUTTON_REGION_GROWING, &C3DProcess::OnBnClickedButtonRegionGrowing)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_CAPTURE, &C3DProcess::OnBnClickedButtonGrowingCapture)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_RECOVERY, &C3DProcess::OnBnClickedButtonGrowingRecovery)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_REMOVE, &C3DProcess::OnBnClickedButtonGrowingRemove)
	ON_BN_CLICKED(IDC_BUTTON_GROWING_CLEAR, &C3DProcess::OnBnClickedButtonGrowingClear)
	ON_BN_CLICKED(IDC_BUTTON_2DSEED_CLEAR, &C3DProcess::OnBnClickedButton2dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_3DSEED_CLEAR, &C3DProcess::OnBnClickedButton3dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_RECORD_LIMIT, &C3DProcess::OnBnClickedButtonRecordLimit)
	ON_BN_CLICKED(IDC_BUTTON_REUSE_LIMIT, &C3DProcess::OnBnClickedButtonReuseLimit)
	ON_BN_CLICKED(IDC_BUTTON_SEED_CHANGE, &C3DProcess::OnBnClickedButtonSeedChange)
	ON_BN_CLICKED(IDC_BUTTON_DILATION, &C3DProcess::OnBnClickedButtonDilation)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_SAVE, &C3DProcess::OnBnClickedButtonVerifySave)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_LOAD, &C3DProcess::OnBnClickedButtonVerifyLoad)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_REUSE, &C3DProcess::OnBnClickedButtonVerifyReuse)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_RECORD, &C3DProcess::OnBnClickedButtonVerifyRecord)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_CALCULATE, &C3DProcess::OnBnClickedButtonVerifyCalculate)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_LINE_ERASE, &C3DProcess::OnBnClickedButtonVerifyLineErase)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_LINE_CLEAR, &C3DProcess::OnBnClickedButtonVerifyLineClear)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_INTERPOLATION, &C3DProcess::OnBnClickedButtonVerifyInterpolation)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_LINE_REFERENCE, &C3DProcess::OnBnClickedButtonVerifyLineReference)
	ON_BN_CLICKED(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE, &C3DProcess::OnBnClickedButtonVerifyLineCancelReference)

	ON_EN_CHANGE(IDC_EDIT_SLICES, &C3DProcess::OnEnChangeEditSlices)
	ON_EN_CHANGE(IDC_EDIT_HU_UP_THRESHOLD, &C3DProcess::OnEnChangeEditHuUpThreshold)
	ON_EN_CHANGE(IDC_EDIT_HU_DOWN_THRESHOLD, &C3DProcess::OnEnChangeEditHuDownThreshold)
	ON_EN_CHANGE(IDC_EDIT_PIXEL_UP_THRESHOLD, &C3DProcess::OnEnChangeEditPixelUpThreshold)
	ON_EN_CHANGE(IDC_EDIT_PIXEL_DOWN_THRESHOLD, &C3DProcess::OnEnChangeEditPixelDownThreshold)
	ON_EN_CHANGE(IDC_EDIT_S_KERNEL, &C3DProcess::OnEnChangeEditSKernel)
	ON_EN_CHANGE(IDC_EDIT_N_KERNEL, &C3DProcess::OnEnChangeEditNKernel)
	ON_EN_CHANGE(IDC_EDIT_PIX_TH, &C3DProcess::OnEnChangeEditPixTh)
	ON_EN_CHANGE(IDC_EDIT_SD_TH, &C3DProcess::OnEnChangeEditSdTh)
	ON_EN_CHANGE(IDC_EDIT_SD_CO, &C3DProcess::OnEnChangeEditSdCo)
	
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
	CString series;	
	series.Format(_T("%d"), m_pDoc->displaySeries);
	SetWindowText("3D Processing - " + 
		m_pDoc->m_dir->PatientName + " - " + series);

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
	GetDlgItem(IDC_BUTTON_MID_FIX)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(FALSE);

	GetDlgItem(IDC_CHECK_SPINE)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_STERNUM)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_SPINE_VERIFY)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK_STERNUM_VERIFY)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_ERASE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_CALCULATE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_RECORD)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_REUSE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_SAVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LOAD)->EnableWindow(FALSE);

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
	const int TotalSlice = Total_Slice;

	if (mpt.x < m_2D_rect.right && mpt.x > m_2D_rect.left && mpt.y < m_2D_rect.bottom && mpt.y > m_2D_rect.top)
	{	// 在 二維 影像視窗範圍內
		if (zDelta < 0)
			DisplaySlice += 1;
		else if (zDelta > 0 && DisplaySlice > 0)
			DisplaySlice -= 1;

		if (DisplaySlice >= TotalSlice)
			DisplaySlice = TotalSlice - 1;
		else if (DisplaySlice < 0)
			DisplaySlice = 0;

		draw_pt_cnt = 0;
		unsigned short& s = DisplaySlice;
		if (m_2Dverify && get_verify_reference)
		{
			if (m_spine_verify)
			{
				draw_spine_pt[s] = draw_spine_pt[verify_reference_slice];
				draw_spine_line[s] = draw_spine_line[verify_reference_slice];
			}
			else if (m_sternum_verify)
			{
				draw_sternum_pt[s] = draw_sternum_pt[verify_reference_slice];
				draw_sternum_line[s] = draw_sternum_line[verify_reference_slice];
			}
		}

		Draw2DImage(DisplaySlice);
		m_ScrollBar.SetScrollPos(DisplaySlice);

		/*if (!spine_vertex.empty())
		{
			TRACE3("%3d slice vertex : x = %3d, y = %3d\n", DisplaySlice, 
				spine_vertex[DisplaySlice][0].first, spine_vertex[DisplaySlice][0].second);
		}*/

		/*if (!spine_edge.empty())
		{
			TRACE3("%3d slice edge : x_min = %3d ; x_max = %3d \n", DisplaySlice, 
				spine_edge[DisplaySlice][0], spine_edge[DisplaySlice][1]);
			TRACE2("y_min = %3d ; y_max = %3d \n",
				spine_edge[DisplaySlice][2], spine_edge[DisplaySlice][3]);
		}*/

	}
	if (mpt.x < m_3D_rect.right && mpt.x > m_3D_rect.left && mpt.y < m_3D_rect.bottom && mpt.y > m_3D_rect.top)
	{	// 在 三維 影像視窗範圍內
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

	draw_pt_cnt = 0;
	unsigned short& s = DisplaySlice;
	if (m_2Dverify && get_verify_reference)
	{
		if (m_spine_verify)
		{
			draw_spine_pt[s] = draw_spine_pt[verify_reference_slice];
			draw_spine_line[s] = draw_spine_line[verify_reference_slice];
		}
		else if (m_sternum_verify)
		{
			draw_sternum_pt[s] = draw_sternum_pt[verify_reference_slice];
			draw_sternum_line[s] = draw_sternum_line[verify_reference_slice];
		}
	}

	Draw2DImage(DisplaySlice);

	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

void C3DProcess::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	// 滑鼠移動事件
	//
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{	// 在 三維 影像視窗
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
		{	// 點選3D種子點功能
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
		if (m_2Dseed)
		{	// 點選2D種子點功能
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


			// 觀察一下點的位置周圍標準差
			std::vector<int> pixel;
			double avg = 0, sd = 0, cnt = 0;
			auto outOfRange = [=](int x, int y, int z)
			{
				if (x < COL && x >= 0 && y < ROW && y >= 0 && z < Total_Slice && z >= 0)
					return false;
				else
					return true;
			};
			auto average = [&](Seed_s s, std::vector<int>& v)
			{	// 計算周圍像素平均
				register int i, j, k;
				for (k = -1; k <= 1; ++k)
				{
					for (j = -1; j <= 1; ++j)
					{
						for (i = -1; i <= 1; ++i)
						{
							if (!outOfRange(s.x + i, s.x + j, s.z + k))
							{
								v.push_back(m_pDoc->m_imgPro[s.z + k][(s.y + j) * COL + (s.x + i)]);
								cnt += 1;
							}
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
			TRACE1("Pixel = %s \n", m_pos_4);
			TRACE3("Cnt = %f, Avg = %f, Sd = %f \n", cnt, avg, sd);
			TRACE1("Judge = %d \n\n", judge[seed_pt.z][seed_pt.y * COL + seed_pt.x]);


			if (m_3Dseed)
				GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(TRUE);

			get_2Dseed = true;
		}
		else if (m_2Dverify)
		{	// 2D驗證功能 (繪製範圍線)
			if (!m_spine_verify && !m_sternum_verify)
			{
				MessageBox("Please Select the Bone !!");
				return;
			}

			unsigned short& s = DisplaySlice;

			std::pair<int, int> click_pt;
			click_pt.first = static_cast<int>(point.x - m_2D_rect.left);
			click_pt.second = static_cast<int>(point.y - m_2D_rect.top);

			// 將要畫線的點，記錄起來
			if (m_spine_verify)
			{
				draw_spine_pt[s].push_back(click_pt);
			}
			else if (m_sternum_verify)
			{
				draw_sternum_pt[s].push_back(click_pt);
			}
			draw_pt_cnt += 1;
			
			// 當點的數目，到達"要把線畫出來"的數目時
			if (draw_pt_cnt == draw_pt_total)
			{
				if (m_spine_verify)
				{
					// 紀錄有畫線的切片且不重複紀錄(QQ，發現應該要用set，可是懶的改)
					auto it = find_if(spine_interpolate_slice.begin(), 
						spine_interpolate_slice.end(),
						[&](const int& p) {return p == s; });
					if (it == spine_interpolate_slice.end())
						spine_interpolate_slice.push_back(s);

					auto len = draw_spine_pt[s].size();

					// 透過內插的方式，把點補齊，連成線
					for (int i = 0; i < draw_pt_total-1; ++i)
					{
						auto start = len - draw_pt_total + i;
						auto end = len - draw_pt_total + i + 1;

						std::pair<int, int> start_pt;
						std::pair<int, int> end_pt;
						start_pt = draw_spine_pt[s].at(start);
						end_pt = draw_spine_pt[s].at(end);

						float seg_x = (end_pt.first - start_pt.first) / 511.0f;
						float seg_y = (end_pt.second - start_pt.second) / 511.0f;

						std::pair<int, int> new_pt;
						for (int j = 0; j <= 511; ++j)
						{
							new_pt.first = static_cast<int>(start_pt.first + j * seg_x);
							new_pt.second = static_cast<int>(start_pt.second + j * seg_y);

							draw_spine_line[s].insert(new_pt);
						}
					}
				}
				else if (m_sternum_verify)
				{
					// 紀錄有畫線的切片且不重複紀錄(QQ，發現應該要用set，可是懶的改)
					auto it = find_if(sternum_interpolate_slice.begin(),
						sternum_interpolate_slice.end(),
						[&](const int p) {return p == s; });
					if (it == sternum_interpolate_slice.end())
						sternum_interpolate_slice.push_back(s);

					auto len = draw_sternum_pt[s].size();

					// 透過內插的方式，把點補齊，連成線
					for (int i = 0; i < draw_pt_total - 1; ++i)
					{
						auto start = len - draw_pt_total + i;
						auto end = len - draw_pt_total + i + 1;

						std::pair<int, int> start_pt;
						std::pair<int, int> end_pt;
						start_pt = draw_sternum_pt[s].at(start);
						end_pt = draw_sternum_pt[s].at(end);

						float seg_x = (end_pt.first - start_pt.first) / 511.0f;
						float seg_y = (end_pt.second - start_pt.second) / 511.0f;

						std::pair<int, int> new_pt;
						for (int j = 0; j <= 511; ++j)
						{
							new_pt.first = static_cast<int>(start_pt.first + j * seg_x);
							new_pt.second = static_cast<int>(start_pt.second + j * seg_y);

							draw_sternum_line[s].insert(new_pt);
						}
					}
				}
				draw_pt_cnt = 0;
			}	// end if 
		}	// end else if

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

void C3DProcess::OnEnChangeEditPixelDownThreshold()
{
	// 更換 二值化 低閾值 (Down Pixel)
	// EditBox : Pixel Threshold (m_pixel_down_threshold)
	//
	UpdateData(TRUE);
	pixel_down_threshold = atoi(m_pixel_down_threshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditPixelUpThreshold()
{
	// 更換 二值化 高閾值 (Up Pixel)
	// EditBox : Pixel Threshold (m_pixel_up_threshold)
	//
	UpdateData(TRUE);
	pixel_up_threshold = atoi(m_pixel_up_threshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditHuDownThreshold()
{
	// 更換 二值化閾值 (Down HU)
	// EditBox : HU Threshold (m_HU_down_threshold)
	//
	UpdateData(TRUE);
	HU_down_threshold = atoi(m_HU_down_threshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditHuUpThreshold()
{
	// 更換 二值化閾值 (Up HU)
	// EditBox : HU Threshold (m_HU_up_threshold)
	//
	UpdateData(TRUE);
	HU_up_threshold = atoi(m_HU_up_threshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditSlices()
{
	// 更換 openGL繪製三維影像的紋理層數
	// EditBox : spine_vertex's slices (m_slices)
	//
	UpdateData(TRUE);
	glSlices = atoi(m_slices);
	Draw3DImage(true);
}

void C3DProcess::OnEnChangeEditSKernel()
{
	// 更換 區域成長標準 種子點kernel
	// EditBox : s_kernel (m_sKernel)
	//
	UpdateData(TRUE);
	RG_term.s_kernel = atoi(m_sKernel);
}

void C3DProcess::OnEnChangeEditNKernel()
{
	// 更換 區域成長標準 種子點周圍像素點的kernel
	// EditBox : n_kernel (m_nKernel)
	//
	UpdateData(TRUE);
	RG_term.n_kernel = atoi(m_nKernel);
}

void C3DProcess::OnEnChangeEditPixTh()
{
	// 更換 區域成長標準 像素閾值
	// EditBox : pix_th (m_pix_th)
	//
	UpdateData(TRUE);
	RG_term.pix_thresh = atoi(m_pix_th);
}

void C3DProcess::OnEnChangeEditSdTh()
{
	// 更換 區域成長標準 標準差閾值
	// EditeBox : sd_th (m_SDth)
	//
	UpdateData(TRUE);
	RG_term.sd_thresh = atoi(m_SDth);
}

void C3DProcess::OnEnChangeEditSdCo()
{
	// 更換 區域成長標準 標準差倍率
	// EditBox : sd_co (m_SDco)
	//
	UpdateData(TRUE);
	RG_term.sd_coeffi = atoi(m_SDco);
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
	if (get_2Dseed)
		m_pos_4.Format("%d", (int)m_pDoc->m_img[seed_pt.z][(seed_pt.y * COL) + seed_pt.x]);

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
	if (get_2Dseed)
		m_pos_4.Format("%d", (int)m_pDoc->m_imgPro[seed_pt.z][(seed_pt.y * COL) + seed_pt.x]);

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

void C3DProcess::OnBnClickedCheckSpineVerify()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D_verify - Spine (m_spine_verify)
	//
	if (!m_spine_verify)
	{	// 打開
		m_spine_verify = TRUE;

	}
	else
	{	// 關閉
		m_spine_verify = FALSE;

	}
	draw_pt_cnt = 0;
	m_sternum_verify = FALSE;
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheckSternumVerify()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D_verify - Sternum (m_sternum_verify)
	//
	if (!m_sternum_verify)
	{	// 打開
		m_sternum_verify = TRUE;

	}
	else
	{	// 關閉
		m_sternum_verify = FALSE;

	}
	draw_pt_cnt = 0;
	m_spine_verify = FALSE;
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheck2dVerify()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D_verify (m_2Dverify)
	//

	if (!m_2Dverify)
	{	// 打開
		m_2Dverify = TRUE;
		
		GetDlgItem(IDC_BUTTON_MID_FIX)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);

		GetDlgItem(IDC_CHECK_SPINE_VERIFY)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_STERNUM_VERIFY)->EnableWindow(TRUE);

		GetDlgItem(IDC_BUTTON_VERIFY_SAVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_LOAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_REUSE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_RECORD)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_CALCULATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_ERASE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_CLEAR)->EnableWindow(TRUE);

		if (get_verify_reference)
		{
			GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(TRUE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
		}
		
	}
	else
	{	// 關閉
		m_2Dverify = FALSE;

		GetDlgItem(IDC_BUTTON_MID_FIX)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);

		GetDlgItem(IDC_CHECK_SPINE_VERIFY)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_STERNUM_VERIFY)->EnableWindow(FALSE);

		GetDlgItem(IDC_BUTTON_VERIFY_SAVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LOAD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_REUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_RECORD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_CALCULATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_ERASE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
	}
	draw_pt_cnt = 0;
	m_2Dseed = FALSE;
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheck2dSeed()
{
	// TODO: Add your control notification handler code here
	// CheckBox : 2D_seed (m_2Dseed)
	//
	
	if (!m_2Dseed)
	{	// 打開
		m_2Dseed = TRUE;
		if (get_2Dseed)
		{
			GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(TRUE);
			if (m_3Dseed)
				GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(TRUE);
		}
		GetDlgItem(IDC_BUTTON_MID_FIX)->EnableWindow(TRUE);

		GetDlgItem(IDC_CHECK_SPINE_VERIFY)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_STERNUM_VERIFY)->EnableWindow(FALSE);

		GetDlgItem(IDC_BUTTON_VERIFY_SAVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LOAD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_REUSE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_RECORD)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_CALCULATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_ERASE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
	}
	else
	{	// 關閉
		m_2Dseed = FALSE;

		GetDlgItem(IDC_BUTTON_MID_FIX)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_2DSEED_CLEAR)->EnableWindow(FALSE);
	}

	m_2Dverify = FALSE;
	UpdateData(FALSE);
	Draw3DImage(true);
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
		GetDlgItem(IDC_CHECK_SPINE)->EnableWindow(TRUE);
		GetDlgItem(IDC_CHECK_STERNUM)->EnableWindow(TRUE);
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
		GetDlgItem(IDC_CHECK_STERNUM)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_SPINE)->EnableWindow(FALSE);
	}
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedCheckSpine()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Spine (m_spine)
	//
	if (m_spine)
	{	// 關閉
		m_spine = FALSE;
		m_result.Format("%lf", 0.0);
		GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	}

	else if (!m_spine)
	{	// 打開
		m_spine = TRUE;
		m_result.Format("%lf", spine_volume);
		if (get_spine)
		{
			GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
		}
	}

	m_sternum = FALSE;
	UpdateData(FALSE);
}

void C3DProcess::OnBnClickedCheckSternum()
{
	// TODO: Add your control notification handler code here
	// CheckBox : Sternum (m_sternum)
	//
	if (m_sternum)			// 關閉
	{
		m_sternum = FALSE;
		m_result.Format("%lf", 0.0);
		GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	}
	else					// 打開
	{
		m_sternum = TRUE;
		m_result.Format("%lf", sternum_volume);
		if (get_sternum)
		{
			GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
		}
	}
	m_spine = FALSE;
	UpdateData(FALSE);
}

void C3DProcess::OnBnClickedButtonVerifyLineErase()
{
	// TODO: Add your control notification handler code here
	// Button : Erase Lines - Erase Verify Line
	//
	if (!m_spine_verify && !m_sternum_verify)
	{
		MessageBox("Please Select the Bone !!");
		return;
	}

	unsigned short& s = DisplaySlice;
	if (m_spine_verify)
	{
		if (!draw_spine_pt.empty())
		{
			if (draw_spine_pt.find(s) != draw_spine_pt.end())
			{
				draw_spine_pt[s].clear();
			}
		}
		if (!draw_spine_line.empty())
		{
			if (draw_spine_line.find(s) != draw_spine_line.end())
			{
				draw_spine_line[s].clear();
			}
		}
		std::vector<int>::iterator it;
		for (it = spine_interpolate_slice.begin(); 
			it < spine_interpolate_slice.end(); ++it)
		{
			if (*it == s)
			{
				spine_interpolate_slice.erase(it);
				break;
			}
		}
	}
	else if (m_sternum_verify)
	{
		if (!draw_sternum_pt.empty())
		{
			if (draw_sternum_pt.find(s) != draw_sternum_pt.end())
			{
				draw_sternum_pt[s].clear();
			}
		}
		if (!draw_sternum_line.empty())
		{
			if (draw_sternum_line.find(s) != draw_sternum_line.end())
			{
				draw_sternum_line[s].clear();
			}
		}
		std::vector<int>::iterator it;
		for (it = sternum_interpolate_slice.begin();
			it < sternum_interpolate_slice.end(); ++it)
		{
			if (*it == s)
			{
				sternum_interpolate_slice.erase(it);
				break;
			}
		}
	}
	draw_pt_cnt = 0;
	Draw2DImage(s);
}

void C3DProcess::OnBnClickedButtonVerifyLineClear()
{
	// TODO: Add your control notification handler code here
	// Button : Clear Lines - Clear Verify Line
	//
	if (!m_spine_verify && !m_sternum_verify)
	{
		MessageBox("Please Select the Bone !!");
		return;
	}
	if (m_spine_verify)
	{
		if (!draw_spine_pt.empty())
		{
			draw_spine_pt.clear();
		}
		if (!draw_spine_line.empty())
		{
			draw_spine_line.clear();
		}
		spine_interpolate_slice.clear();
	}
	else if (m_sternum_verify)
	{
		if (!draw_sternum_pt.empty())
		{
			draw_sternum_pt.clear();
		}
		if (!draw_sternum_line.empty())
		{
			draw_sternum_line.clear();
		}
		sternum_interpolate_slice.clear();
	}
	draw_pt_cnt = 0;
	verify_reference_slice = 0;
	get_verify_reference = false;
	Draw2DImage(DisplaySlice);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
}

void C3DProcess::OnBnClickedButtonVerifyLineReference()
{
	// TODO: Add your control notification handler code here
	// Button : Reference - Reference displaySlice's Line
	// 
	get_verify_reference = true;
	verify_reference_slice = DisplaySlice;
	if (m_spine_verify)
	{
		if (draw_spine_pt.find(DisplaySlice) == draw_spine_pt.end())
			draw_spine_pt[DisplaySlice] = {};
		if (draw_spine_line.find(DisplaySlice) == draw_spine_line.end())
			draw_spine_line[DisplaySlice] = {};
	}
	else if (m_sternum_verify)
	{
		if (draw_sternum_pt.find(DisplaySlice) == draw_sternum_pt.end())
			draw_sternum_pt[DisplaySlice] = {};
		if (draw_sternum_line.find(DisplaySlice) == draw_sternum_line.end())
			draw_sternum_line[DisplaySlice] = {};
	}
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(TRUE);
}

void C3DProcess::OnBnClickedButtonVerifyLineCancelReference()
{
	// TODO: Add your control notification handler code here
	// Button : Cancel Ref. - Cancel reference line
	//
	get_verify_reference = false;
	verify_reference_slice = 0;
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
}

void C3DProcess::OnBnClickedButtonVerifyInterpolation()
{
	// TODO: Add your control notification handler code here
	// Button : Calculate - Interpolation
	//
	const int row = ROW;
	const int col = COL;
	const int total_slice = Total_Slice;
	
	// 先判斷切片有無繪製驗證線
	if (m_spine_verify)
	{
		if (spine_interpolate_slice.size() <= 1) return;
	}
	else if (m_sternum_verify)
	{
		if (sternum_interpolate_slice.size() <= 1) return;
	}

	clock_t start, end;
	start = clock();

	// 將有畫限制線的切片的線延伸到影像邊界
	if (m_spine_verify)
	{
		// 檢查每一張有畫spine_line的切片
		for (auto& n : spine_interpolate_slice)
		{
			int x_start = draw_spine_line[n].begin()->first;
			int y_start = draw_spine_line[n].begin()->second;
			int x_end = draw_spine_line[n].rbegin()->first;
			int y_end = draw_spine_line[n].rbegin()->second;

			if (x_start != 0)
			{
				for (int i = 0; i <= x_start; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_start;
					draw_spine_line[n].insert(pt);
				}
			}
			if (x_end != 511)
			{
				for (int i = x_end; i <= 511; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_end;
					draw_spine_line[n].insert(pt);
				}
			}
			std::pair<int, int> pt;
			for (int i = 0; i < 511; ++i)
			{
				auto it = std::find_if(draw_spine_line[n].begin(), draw_spine_line[n].end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				if (it == draw_spine_line[n].end())
				{
					std::pair<int, int> new_pt;
					new_pt.first = i;
					new_pt.second = pt.second;
					draw_spine_line[n].insert(new_pt);
				}
				else
					pt = *it;
			}
		}
	}
	else if (m_sternum_verify)
	{
		// 檢查每一張有畫sternum_line的切片
		for (auto& n : sternum_interpolate_slice)
		{
			int x_start = draw_sternum_line[n].begin()->first;
			int y_start = draw_sternum_line[n].begin()->second;
			int x_end = draw_sternum_line[n].rbegin()->first;
			int y_end = draw_sternum_line[n].rbegin()->second;

			if (x_start != 0)
			{
				for (int i = 0; i <= x_start; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_start;
					draw_sternum_line[n].insert(pt);
				}
			}
			if (x_end != 511)
			{
				for (int i = x_end; i <= 511; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_end;
					draw_sternum_line[n].insert(pt);
				}
			}
			std::pair<int, int> pt;
			for (int i = 0; i < 511; ++i)
			{
				auto it = std::find_if(draw_sternum_line[n].begin(), draw_sternum_line[n].end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				if (it == draw_sternum_line[n].end())
				{
					std::pair<int, int> new_pt;
					new_pt.first = i;
					new_pt.second = pt.second;
					draw_sternum_line[n].insert(new_pt);
				}
				else
					pt = *it;
			}
		}
	}

	// 內插切片與切片之間的線
	if (m_spine_verify)
	{
		// 從第一個紀錄的切片開始，依序與後一個紀錄的切片之間進行內插
		auto len = spine_interpolate_slice.size();
		for (int n = 0; n < len - 1; ++n)
		{
			// 從左到右，0~511，依序在每一張切片內插
			for (int i = 0; i < row; ++i)
			{
				// 計算現在的切片與後一個切片之間，Y 的距離
				int start = spine_interpolate_slice[n];
				int end = spine_interpolate_slice[n + 1];

				auto it_start = std::find_if(draw_spine_line[start].begin(), draw_spine_line[start].end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				auto it_end= std::find_if(draw_spine_line[end].begin(), draw_spine_line[end].end(),
					[&](const pair<int, int>& p) {return p.first == i; });

				int y_start = (*it_start).second;
				int y_end = (*it_end).second;
				double y_seg = static_cast<double>(y_end - y_start) / static_cast<double>(end - start);

				// 內插每一張切片上，某一 X 行上的 Y 點
				for (int s = start + 1; s < end; ++s)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = static_cast<int>(y_start + (s - start) * y_seg);
					draw_spine_line[s].insert(pt);
				}
			}
		}
		TRACE1("Verify Interpolation Slice Count : %d. \n", len);
	}
	else if (m_sternum_verify)
	{
		// 從第一個紀錄的切片開始，依序與後一個紀錄的切片之間進行內插
		auto len = sternum_interpolate_slice.size();
		for (int n = 0; n < len - 1; ++n)
		{
			// 從左到右，0~511，依序在每一張切片內插
			for (int i = 0; i < row; ++i)
			{
				// 計算現在的切片與後一個切片之間，Y 的距離
				int start = sternum_interpolate_slice[n];
				int end = sternum_interpolate_slice[n + 1];

				auto it_start = std::find_if(draw_sternum_line[start].begin(), draw_sternum_line[start].end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				auto it_end = std::find_if(draw_sternum_line[end].begin(), draw_sternum_line[end].end(),
					[&](const pair<int, int>& p) {return p.first == i; });

				int y_start = (*it_start).second;
				int y_end = (*it_end).second;
				double y_seg = static_cast<double>(y_end - y_start) / static_cast<double>(end - start);

				// 內插每一張切片上，某一 X 行上的 Y 點
				for (int s = start + 1; s < end; ++s)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = static_cast<int>(y_start + (s - start) * y_seg);
					draw_sternum_line[s].insert(pt);
				}
			}
		}
		TRACE1("Verify Interpolation Slice Count : %d. \n", len);
	}

	end = clock();
	TRACE1("Verify Interpolation Time : %f (s) \n\n", (double)(end - start) / CLOCKS_PER_SEC);

	Draw2DImage(DisplaySlice);

}

void C3DProcess::OnBnClickedButtonVerifyCalculate()
{
	// TODO: Add your control notification handler code here
	// Button : Calculate - Verify volume calculate
	//
	clock_t start, end;
	size_t spine_line_size = draw_spine_line.size();
	size_t sternum_line_size = draw_sternum_line.size();
	size_t totalSlice = static_cast<size_t>(Total_Slice);

	if (spine_line_size != totalSlice &&
		sternum_line_size != totalSlice )
	{
		MessageBox("Please interpolation or draw the line completely !!");
		return;
	}

	start = clock();
	double volume = 0.0;
	const int row = ROW;
	const int col = COL;
	if (m_spine_verify)
	{	// 檢查 spine_line 每張切片的線
		// 讓每條線延伸到影像邊界
		for (auto& n : draw_spine_line)
		{
			int x_start = n.second.begin()->first;
			int y_start = n.second.begin()->second;
			int x_end = n.second.rbegin()->first;
			int y_end = n.second.rbegin()->second;
			if (x_start != 0)
			{
				for (int i = 0; i <= x_start; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_start;
					n.second.insert(pt);
				}
			}
			if (x_end != 511)
			{
				for (int i = x_end; i <= 511; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_end;
					n.second.insert(pt);
				}
			}
			std::pair<int, int> pt;
			for (int i = 0; i < 511; ++i)
			{
				auto it = std::find_if(n.second.begin(), n.second.end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				if (it == n.second.end())
				{
					std::pair<int, int> new_pt;
					new_pt.first = i;
					new_pt.second = pt.second;
					n.second.insert(new_pt);
				}
				else
					pt = *it;
			}
		}	// end for

		// 計算每張切片，範圍內，符合HU閥值的像素數目
		int n = 0;
		unsigned long long cnt = 0;
		while (n < totalSlice)
		{
			for (int i = 0; i < col; ++i)
			{
				auto it = std::find_if(draw_spine_line[n].begin(),
					draw_spine_line[n].end(),
					[&](const pair<int, int>& p) {return p.first == i;} );

				int y_start = it->second;
				for (int j = y_start; j < row; ++j)
				{
					if (m_pDoc->m_HUimg[n][j * col + i] >= HU_down_threshold &&
						m_pDoc->m_HUimg[n][j * col + i] <= HU_up_threshold)
					{
						cnt += 1;
					}
				}
			}
			n += 1;
		}
		volume = static_cast<double>(
			(cnt * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z) / 1000.0);
		TRACE1("Spine Verify Volume = %lf. \n", volume);
	}
	else if (m_sternum_verify)
	{	// 檢查 sternum_line 每張切片的線
		// 讓每條線延伸到影像邊界
		for (auto& n : draw_sternum_line)
		{
			int x_start = n.second.begin()->first;
			int y_start = n.second.begin()->second;
			int x_end = n.second.rbegin()->first;
			int y_end = n.second.rbegin()->second;
			if (x_start != 0)
			{
				for (int i = 0; i < x_start; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_start;
					n.second.insert(pt);
				}
			}
			if (x_end != 511)
			{
				for (int i = x_end; i <= 511; ++i)
				{
					std::pair<int, int> pt;
					pt.first = i;
					pt.second = y_end;
					n.second.insert(pt);
				}
			}
			std::pair<int, int> pt;
			for (int i = 0; i < 511; ++i)
			{
				auto it = std::find_if(n.second.begin(), n.second.end(),
					[&](const pair<int, int>& p) {return p.first == i; });
				if (it == n.second.end())
				{
					std::pair<int, int> new_pt;
					new_pt.first = i;
					new_pt.second = pt.second;
					n.second.insert(new_pt);
				}
				else
					pt = *it;
			}
		}	// end for

		// 計算每張切片，範圍內，符合HU閥值的像素數目
		int n = 0;
		unsigned long long cnt = 0;
		while (n < totalSlice)
		{
			for (int i = 0; i < col; ++i)
			{
				auto it = std::find_if(draw_sternum_line[n].begin(),
					draw_sternum_line[n].end(),
					[&](const std::pair<int, int>& p) {return p.first == i;} );

				int y_end = it->second;
				for (int j = 0; j < y_end; ++j)
				{
					if (m_pDoc->m_HUimg[n][j * col + i] >= HU_down_threshold &&
						m_pDoc->m_HUimg[n][j * col + i] <= HU_up_threshold)
					{
						cnt += 1;
					}
				}
			}
			n += 1;
		}
		volume = static_cast<double>(
			(cnt * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z) / 1000.0);
		TRACE1("Sternum Verify Volume = %lf. \n", volume);
	}

	end = clock();
	draw_pt_cnt = 0;
	verify_reference_slice = 0;
	get_verify_reference = false;
	m_result_2.Format("%lf", volume);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_REFERENCE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_VERIFY_LINE_CANCEL_REFERENCE)->EnableWindow(FALSE);
	TRACE1("Verify Calculate Time : %f (s) \n\n", (double)(end - start) / CLOCKS_PER_SEC);
	Draw2DImage(DisplaySlice);
	UpdateData(FALSE);
}

void C3DProcess::OnBnClickedButtonVerifyRecord()
{
	// TODO: Add your control notification handler code here
	// Button : Record Lines (Verify Lines Record)
	//
	if (!draw_spine_pt.empty())
	{
		if (!m_pDoc->draw_spine_pt.empty())
			m_pDoc->draw_spine_pt.clear();
		m_pDoc->draw_spine_pt = draw_spine_pt;
		TRACE("Verify Spine Point has been record ! \n");
	}
	if (!draw_spine_line.empty())
	{
		if (!m_pDoc->draw_spine_line.empty())
			m_pDoc->draw_spine_line.clear();
		m_pDoc->draw_spine_line = draw_spine_line;
		TRACE("Verify Spine Line has been record ! \n");
	}
	if (!draw_sternum_pt.empty())
	{
		if (!m_pDoc->draw_sternum_pt.empty())
			m_pDoc->draw_sternum_pt.clear();
		m_pDoc->draw_sternum_pt = draw_sternum_pt;
		TRACE("Verify Sternum Point has been record ! \n");
	}
	if (!draw_sternum_line.empty())
	{
		if (!m_pDoc->draw_sternum_line.empty())
			m_pDoc->draw_sternum_line.clear();
		m_pDoc->draw_sternum_line = draw_sternum_line;
		TRACE("Verify Sternum Line has been record ! \n");
	}
}

void C3DProcess::OnBnClickedButtonVerifyReuse()
{
	// TODO: Add your control notification handler code here
	// Button : Reuse Lines (Verify Lines Reuse)
	//
	if (!m_pDoc->draw_spine_pt.empty())
	{
		if (!draw_spine_pt.empty())
			draw_spine_pt.clear();
		draw_spine_pt = m_pDoc->draw_spine_pt;
		TRACE("Verify Spine Point has been reuse ! \n");
	}
	if (!m_pDoc->draw_spine_line.empty())
	{
		if (!draw_spine_line.empty())
			draw_spine_line.clear();
		draw_spine_line = m_pDoc->draw_spine_line;
		TRACE("Verify Spine Line has been reuse ! \n");
	}
	if (!m_pDoc->draw_sternum_pt.empty())
	{
		if (!draw_sternum_pt.empty())
			draw_sternum_pt.clear();
		draw_sternum_pt = m_pDoc->draw_sternum_pt;
		TRACE("Verify Spine Point has been reuse ! \n");
	}
	if (!m_pDoc->draw_sternum_line.empty())
	{
		if (!draw_sternum_line.empty())
			draw_sternum_line.clear();
		draw_sternum_line = m_pDoc->draw_sternum_line;
		TRACE("Verify Spine Point has been reuse ! \n");
	}
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnBnClickedButtonVerifySave()
{
	// TODO: Add your control notification handler code here
	// Button : Save Lines (Verify Lines Save)
	//
	CString dir_path_name = m_pDoc->m_dir->DirPathName;
	CString dir_file_name = m_pDoc->m_dir->DirFileName;
	CString path = dir_path_name.Left(dir_path_name.Find(dir_file_name));
	CString series_number = m_pDoc->m_dir->SeriesList[m_pDoc->displaySeries]->SeriesNumber;
	CString save_path_root = path + "Data\\";
	CreateDirectory(save_path_root, NULL);
	save_path_root += series_number + "\\";
	CreateDirectory(save_path_root, NULL);

	CFile fp;
	
	// 儲存 Spine Verify Point
	if (!draw_spine_pt.empty())
	{
		CString save_path = save_path_root + "Verify_Spine_Pt\\";
		CreateDirectory(save_path, NULL);
		for (const auto& n : draw_spine_pt)
		{
			CString file;	file.Format("%d", n.first);
			CString file_name = save_path + file;
			if (fp.Open(file_name, CFile::modeCreate | 
				CFile::modeWrite | CFile::typeBinary))
			{	
				int buffer;
				for (const auto& p : n.second)
				{
					buffer = p.first;
					fp.Write(&buffer, sizeof(int));
					buffer = p.second;
					fp.Write(&buffer, sizeof(int));
				}
				fp.Close();
			}
		}
		TRACE("Spine Verify Points Save Success ! \n");
	}

	// 儲存 Spine Verify Line
	if (!draw_spine_line.empty())
	{
		CString save_path = save_path_root + "Verify_Spine_Line\\";
		CreateDirectory(save_path, NULL);
		for (const auto& n : draw_spine_line)
		{
			CString file;	file.Format("%d", n.first);
			CString file_name = save_path + file;
			if (fp.Open(file_name, CFile::modeCreate |
			CFile::modeWrite | CFile::typeBinary))
			{
				int buffer;
				for (const auto& p : n.second)
				{
					buffer = p.first;
					fp.Write(&buffer, sizeof(int));
					buffer = p.second;
					fp.Write(&buffer, sizeof(int));
				}
				fp.Close();
			}
		}
		TRACE("Spine Verify Lines Save Success ! \n");
	}

	// 儲存 Sternum Verify Point
	if (!draw_sternum_pt.empty())
	{
		CString save_path = save_path_root + "Verify_Sternum_Pt\\";
		CreateDirectory(save_path, NULL);
		for (const auto& n : draw_sternum_pt)
		{
			CString file;	file.Format("%d", n.first);
			CString file_name = save_path + file;
			if (fp.Open(file_name, CFile::modeCreate |
				CFile::modeWrite | CFile::typeBinary))
			{
				int buffer;
				for (const auto& p : n.second)
				{
					buffer = p.first;
					fp.Write(&buffer, sizeof(int));
					buffer = p.second;
					fp.Write(&buffer, sizeof(int));
				}
				fp.Close();
			}
		}
		TRACE("Sternum Verify Points Save Success ! \n");
	}

	// 儲存 Sternum Verify Line
	if (!draw_sternum_line.empty())
	{
		CString save_path = save_path_root + "Verify_Sternum_Line\\";
		CreateDirectory(save_path, NULL);
		for (const auto& n : draw_sternum_line)
		{
			CString file;	file.Format("%d", n.first);
			CString file_name = save_path + file;
			if (fp.Open(file_name, CFile::modeCreate |
				CFile::modeWrite | CFile::typeBinary))
			{
				int buffer;
				for (const auto& p : n.second)
				{
					buffer = p.first;
					fp.Write(&buffer, sizeof(int));
					buffer = p.second;
					fp.Write(&buffer, sizeof(int));
				}
				fp.Close();
			}
		}
		TRACE("Sternum Verify Lines Save Success ! \n");
	}

}

void C3DProcess::OnBnClickedButtonVerifyLoad()
{
	// TODO: Add your control notification handler code here
	// Button : Load Lines (Verify Lines Load)
	//
	CString dir_path_name = m_pDoc->m_dir->DirPathName;
	CString dir_file_name = m_pDoc->m_dir->DirFileName;
	CString path = dir_path_name.Left(dir_path_name.Find(dir_file_name));
	CString series_number = m_pDoc->m_dir->SeriesList[m_pDoc->displaySeries]->SeriesNumber;
	CString save_path_root = path + "Data\\" + series_number;

	CFile fp;
	CFileFind finder;
	CString save_path;	BOOL exist;
	std::vector<CString> file_list;
	if (!finder.FindFile(save_path_root)) return;
	
	// 讀取 Verify Spine Point
	//
	// 確認並搜索路徑底下的 Data
	save_path = save_path_root + "\\Verify_Spine_Pt";
	exist = finder.FindFile(save_path + "\\*.*");
	while (exist)
	{
		exist = finder.FindNextFileA();
		if (finder.IsDots()) continue;
		file_list.push_back(finder.GetFileName());
	}
	// 開檔
	if (!file_list.empty())
	{
		// 先清空並釋放記憶體
		for (auto& n : draw_spine_pt)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		draw_spine_pt.swap(empty_map);
		draw_spine_pt.clear();

		// 逐一讀取每個檔案
		for (const auto& n : file_list)
		{
			int s = atoi(n);
			CString file_name = save_path + "\\" + n;
			if (fp.Open(file_name, CFile::modeRead | 
				CFile::typeBinary))
			{
				int buffer;
				std::pair<int, int> pt;
				auto size = fp.GetLength();
				auto pos = fp.GetPosition();
				while (pos < size)
				{
					fp.Read(&buffer, sizeof(int));
					pt.first = buffer;
					fp.Read(&buffer, sizeof(int));
					pt.second = buffer;
					draw_spine_pt[s].push_back(pt);
					pos = fp.GetPosition();
				}
				fp.Close();
			}
		}
		file_list.clear();
		TRACE("Spine Verify Points Load Success ! \n");
	}

	// 讀取 Verify Spine Line
	//
	// 確認並搜索路徑底下的 Data
	save_path = save_path_root + "\\Verify_Spine_Line";
	exist = finder.FindFile(save_path + "\\*.*");
	while (exist)
	{
		exist = finder.FindNextFileA();
		if (finder.IsDots()) continue;
		file_list.push_back(finder.GetFileName());
	}
	// 開檔
	if (!file_list.empty())
	{
		// 先清空並釋放記憶體
		for (auto& n : draw_spine_line)
		{
			n.second.clear();
			std::set<std::pair<int, int>> empty_set;
			n.second.swap(empty_set);
		}
		std::map<int, std::set<std::pair<int, int>>> empty_map;
		draw_spine_line.swap(empty_map);
		draw_spine_line.clear();

		// 逐一讀取每個檔案
		for (const auto& n : file_list)
		{
			int s = atoi(n);
			CString file_name = save_path + "\\" + n;
			if (fp.Open(file_name, CFile::modeRead |
				CFile::typeBinary))
			{
				int buffer;
				std::pair<int, int> pt;
				auto size = fp.GetLength();
				auto pos = fp.GetPosition();
				while (pos < size)
				{
					fp.Read(&buffer, sizeof(int));
					pt.first = buffer;
					fp.Read(&buffer, sizeof(int));
					pt.second = buffer;
					draw_spine_line[s].insert(pt);
					pos = fp.GetPosition();
				}
				fp.Close();
			}
		}
		file_list.clear();
		TRACE("Spine Verify Lines Load Success ! \n");
	}

	// 讀取 Verify Sternum Point
	//
	// 確認並搜索路徑底下的 Data
	save_path = save_path_root + "\\Verify_Sternum_Pt";
	exist = finder.FindFile(save_path + "\\*.*");
	while (exist)
	{
		exist = finder.FindNextFileA();
		if (finder.IsDots()) continue;
		file_list.push_back(finder.GetFileName());
	}
	// 開檔
	if (!file_list.empty())
	{
		// 先清空並釋放記憶體
		for (auto& n : draw_sternum_pt)
		{
			n.second.clear();
			n.second.shrink_to_fit();
		}
		std::map<int, std::vector<std::pair<int, int>>> empty_map;
		draw_sternum_pt.swap(empty_map);
		draw_sternum_pt.clear();

		// 逐一讀取每個檔案
		for (const auto& n : file_list)
		{
			int s = atoi(n);
			CString file_name = save_path + "\\" + n;
			if (fp.Open(file_name, CFile::modeRead |
				CFile::typeBinary))
			{
				int buffer;
				std::pair<int, int> pt;
				auto size = fp.GetLength();
				auto pos = fp.GetPosition();
				while (pos < size)
				{
					fp.Read(&buffer, sizeof(int));
					pt.first = buffer;
					fp.Read(&buffer, sizeof(int));
					pt.second = buffer;
					draw_sternum_pt[s].push_back(pt);
					pos = fp.GetPosition();
				}
				fp.Close();
			}
		}
		file_list.clear();
		TRACE("Sternum Verify Points Load Success ! \n");
	}

	// 讀取 Verify Sternum Line
	//
	// 確認並搜索路徑底下的 Data
	save_path = save_path_root + "\\Verify_Sternum_Line";
	exist = finder.FindFile(save_path + "\\*.*");
	while (exist)
	{
		exist = finder.FindNextFileA();
		if (finder.IsDots()) continue;
		file_list.push_back(finder.GetFileName());
	}
	// 開檔
	if (!file_list.empty())
	{
		// 先清空並釋放記憶體
		for (auto& n : draw_sternum_line)
		{
			n.second.clear();
			std::set<std::pair<int, int>> empty_set;
			n.second.swap(empty_set);
		}
		std::map<int, std::set<std::pair<int, int>>> empty_map;
		draw_sternum_line.swap(empty_map);
		draw_sternum_line.clear();

		// 逐一讀取每個檔案
		for (const auto& n : file_list)
		{
			int s = atoi(n);
			CString file_name = save_path + "\\" + n;
			if (fp.Open(file_name, CFile::modeRead |
				CFile::typeBinary))
			{
				int buffer;
				std::pair<int, int> pt;
				auto size = fp.GetLength();
				auto pos = fp.GetPosition();
				while (pos < size)
				{
					fp.Read(&buffer, sizeof(int));
					pt.first = buffer;
					fp.Read(&buffer, sizeof(int));
					pt.second = buffer;
					draw_sternum_line[s].insert(pt);
					pos = fp.GetPosition();
				}
				fp.Close();
			}
		}
		file_list.clear();
		TRACE("Sternum Verify Lines Load Success ! \n");
	}

	finder.Close();
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

void C3DProcess::OnBnClickedExit()
{
	// TODO: Add your control notification handler code here
	// Button : Exit
	//
	CDialogEx::OnOK();
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

void C3DProcess::OnBnClickedButtonMidFix()
{
	// TODO: Add your control notification handler code here
	// Button : Mid_Fix
	//
	if (get_2Dseed)
	{
		mid_fix_pt.first = seed_pt.x;
		mid_fix_pt.second = seed_pt.y;
		get_mid_fix = true;
	}
	else
	{
		mid_fix_pt = std::make_pair(0, 0);
		get_mid_fix = false;
	}

}

void C3DProcess::OnBnClickedButtonReuseLimit()
{
	// TODO: Add your control notification handler code here
	// Button : Reuse Limit (vertex.edge.line)
	//
	if (!m_pDoc->spine_vertex.empty())
	{
		if (!spine_vertex.empty())
			spine_vertex.clear();
		spine_vertex = m_pDoc->spine_vertex;
		TRACE("Spine Vertex has been reuse ! \n");
	}
	if (!m_pDoc->spine_line.empty())
	{
		if (!spine_line.empty())
			spine_line.clear();
		spine_line = m_pDoc->spine_line;
		TRACE("Spine Line has been reuse ! \n");
	}
	if (!m_pDoc->spine_edge.empty())
	{
		if (!spine_edge.empty())
			spine_edge.clear();
		spine_edge = m_pDoc->spine_edge;
		TRACE("Spine Edge has been reuse ! \n");
	}
	if (!m_pDoc->sternum_vertex.empty())
	{
		if (!sternum_vertex.empty())
			sternum_vertex.clear();
		sternum_vertex = m_pDoc->sternum_vertex;
		TRACE("Sternum Vertex has been reuse ! \n");
	}
	if (!m_pDoc->sternum_line.empty())
	{
		if (!sternum_line.empty())
			sternum_line.clear();
		sternum_line = m_pDoc->sternum_line;
		TRACE("Sternum Line has been reuse ! \n");
	}
	if (!m_pDoc->sternum_edge.empty())
	{
		if (!sternum_edge.empty())
			sternum_edge.clear();
		sternum_edge = m_pDoc->sternum_edge;
		TRACE("Sternum Edge has been reuse ! \n");
	}
}

void C3DProcess::OnBnClickedButtonRecordLimit()
{
	// TODO: Add your control notification handler code here
	// Button : Record Limit (vertex.edge.line)
	//
	if (!spine_vertex.empty())
	{
		if (!m_pDoc->spine_vertex.empty())
			m_pDoc->spine_vertex.clear();
		m_pDoc->spine_vertex = spine_vertex;
		TRACE("Spine Vertex has been record ! \n");
	}
	if (!spine_line.empty())
	{
		if (!m_pDoc->spine_line.empty())
			m_pDoc->spine_line.clear();
		m_pDoc->spine_line = spine_line;
		TRACE("Spine Line has been record ! \n");
	}
	if (!spine_edge.empty())
	{
		if (!m_pDoc->spine_edge.empty())
			m_pDoc->spine_edge.clear();
		m_pDoc->spine_edge = spine_edge;
		TRACE("Spine Edge has been record ! \n");
	}
	if (!sternum_vertex.empty())
	{
		if (!m_pDoc->sternum_vertex.empty())
			m_pDoc->sternum_vertex.clear();
		m_pDoc->sternum_vertex = sternum_vertex;
		TRACE("Sternum Vertex has been record ! \n");
	}
	if (!sternum_line.empty())
	{
		if (!m_pDoc->sternum_line.empty())
			m_pDoc->sternum_line.clear();
		m_pDoc->sternum_line = sternum_line;
		TRACE("Sternum Line has been record ! \n");
	}
	if (!sternum_edge.empty())
	{
		if (!m_pDoc->sternum_edge.empty())
			m_pDoc->sternum_edge.clear();
		m_pDoc->sternum_edge = sternum_edge;
		TRACE("Sternum Edge has been record ! \n");
	}
}

void C3DProcess::OnBnClickedButtonRegionGrowing()
{
	// TODO: Add your control notification handler code here
	// Button : 3D Region Growing
	//
	if (!get_3Dseed)	return;
	if (!m_spine && !m_sternum)
	{
		MessageBox("Please select the bone !!");
		return;
	}
	
	clock_t start, end;
	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("Region growing...");
	
	// 宣告 成長標準 參數
	//
	RG_term.seed = seed_img;

	// 執行 3D_Region growing
	//
	if (m_spine)
	{
		start = clock();
		if (!spine_line.empty())
			RG_3D_Spine_process(judge, RG_term);
		else
			RG_3D_ProposedMethod(judge, RG_term);
			//RG_3D_GlobalAvgConnected(judge, RG_term);
			//RG_3D_ConfidenceConnected(judge, RG_term);
		end = clock();
		spine_volume = Calculate_Volume(judge);
		m_result.Format("%lf", spine_volume);
		get_regionGrow = true;
		get_spine = true;
	}
	else if (m_sternum)
	{
		start = clock();
		RG_3D_ProposedMethod(judge, RG_term);
		//RG_3D_ConfidenceConnected(judge, RG_term);
		//RG_3D_GlobalAvgConnected(judge, RG_term);
		end = clock();
		sternum_volume = Calculate_Volume(judge);
		m_result.Format("%lf", sternum_volume);
		get_regionGrow = true;
		get_sternum = true;
	}
	
	TRACE("Growing Volume : " + m_result + " (cm3) \n");
	TRACE1("RG Time : %f (s) \n\n", (double)(end - start) / CLOCKS_PER_SEC);
	
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

void C3DProcess::OnBnClickedButtonDilation()
{
	// TODO: Add your control notification handler code here
	// Button : Dilation (目前為 未定型的處理)
	//
	if (!get_regionGrow)	return;

	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("2nd Region growing...");

	clock_t start = clock();
	if (m_spine && get_spine)
	{
		if (!spine_vertex.empty())	spine_vertex.clear();
		if (!spine_edge.empty()) spine_edge.clear();
		if (!spine_line.empty()) spine_line.clear();
		Spine_process();

		// 二次區域成長
		UpdateData(TRUE);
		RG_term.seed = seed_img;
		RG2_3D_Spine_process(judge, RG_term);

		Dilation_3D(judge, 26);
		//Spine_process_fix();
		spine_volume = Calculate_Volume(judge);
		m_result.Format("%lf", spine_volume);
	}
	else if (m_sternum && get_sternum)
	{
		if (sternum_vertex.empty()) sternum_vertex.clear();
		if (sternum_edge.empty()) sternum_edge.clear();
		if (sternum_line.empty()) sternum_line.clear();
		Sternum_process();

		// 二次區域成長
		UpdateData(TRUE);
		RG_term.seed = seed_img;
		RG2_3D_Sternum_process(judge, RG_term);

		//Dilation_3D(judge, 26);
		//Sternum_process_fix();
		sternum_volume = Calculate_Volume(judge);
		m_result.Format("%lf", sternum_volume);
	}
	clock_t end = clock();

	PrepareVolume();
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);

	TRACE("Growing Volume : " + m_result + " (cm3) \n");
	TRACE1("2nd process Time : %f (s)\n\n", (double)(end - start) / CLOCKS_PER_SEC);
	m_wait->DestroyWindow();
	UpdateData(FALSE);
	delete m_wait;

}

void C3DProcess::Spine_process()
{
	// 脊椎進行二次成長前的預處理
	//
	const int obj = 1;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	BYTE**& org = m_pDoc->m_img;
	BYTE**& pro = m_pDoc->m_imgPro;

	// 尋找每張slice的三角頂點
	auto findVertex = [&](int start_slice)
	{
		int pos, x, y;
		int s = start_slice;
		std::vector<int> x_pos;
		std::vector<int> y_pos;
		x_pos.reserve(20000);
		y_pos.reserve(20000);

		while (s < totalSlice)
		{
			pos = 0;
			int ver_mid = 0, y_min = 512;
			int ver_lft = 0, ver_rht = 0;

			// 紀錄每個種子點的x.y座標值
			for (x = 0; x < col; ++x)
			{
				for (y = 0; y < row; ++y)
				{
					pos = y * col + x;
					if (judge[s][pos] == obj)
					{
						x_pos.push_back(x);
						y_pos.push_back(y);

						// 尋找-中上點(中間最高)
						if (x < 350 && y < y_min)
						{
							y_min = y;
							ver_mid = pos;
						}
						// 尋找-左下點(第一個接觸到的點)
						if (ver_lft == 0)
							ver_lft = pos;
					}
				}
			}

			// 如果成長範圍太少導致判別會錯誤，就先跳過
			if (x_pos.size() < 10 || y_pos.size() < 10)
			{
				TRACE1("Slice : %3d cannot find vertex. \n", s);
				x_pos.clear();
				y_pos.clear();
				s += 2;
				continue;
			}
			size_t x_len = x_pos.size();
			size_t y_len = y_pos.size();

			// 尋找-右下點(該行最先接觸到的點)
			//ver_rht = y_pos.at(y_len - 1) * col + x_pos.at(x_len - 1);
			x = x_pos.at(x_len - 1); y = 0;
			while (y < row)
			{
				if (judge[s][y * col + x] == obj)
					break;
				y++;
			}
			ver_rht = y * col + x;

			spine_vertex[s].assign(3, std::make_pair(0, 0));
			// 存三角頂點 - 中上點
			spine_vertex[s].at(0) = std::make_pair((ver_mid % col), (ver_mid / col));
			// 存三角頂點 - 左下點
			spine_vertex[s].at(1) = std::make_pair((ver_lft % col), (ver_lft / col));
			// 計算三角頂點 - 右下點 (避免成長到影像一半以上的肋骨)
			if ((ver_rht / col) <= (ver_mid / col) || (ver_rht % col) - (ver_mid % col) >= 200)
			{
				x = (ver_lft % col) + 2 * ((ver_mid % col) - (ver_lft % col));
				y = ver_lft / col;
				spine_vertex[s].at(2) = std::make_pair(x, y);
			}
			else
				spine_vertex[s].at(2) = std::make_pair((ver_rht % col), (ver_rht / col));

			x_pos.clear();
			y_pos.clear();
			s += 2;
		}	// end while

		x_pos.shrink_to_fit();
		y_pos.shrink_to_fit();
		if (start_slice == 0)	TRACE("Find Vertex : Even Slice Success ! \n");
		else TRACE("Find Vertex : Odd Slice Success ! \n");
	};	// end findVertex()
	
	thread th_0(findVertex, 0);
	thread th_1(findVertex, 1);
	th_0.join(); th_1.join();

	// 修正沒有判斷到頂點的slice
	int n = 0;
	if (spine_vertex.find(n) == spine_vertex.end() || spine_vertex[n].size() < 3)
	{
		int s = n + 1;
		// 直到找到有頂點的slice為止
		while (spine_vertex.find(s) == spine_vertex.end() || spine_vertex[s].size() < 3)
			++s;
		for (; n < s; ++n)
		{
			spine_vertex[n].assign(3, std::make_pair(0, 0));
			spine_vertex[n] = spine_vertex[s];
		}
		TRACE("0 slice's vertex have been fixed ! \n");
	}
	else 
		TRACE("0 slice have vertex ! \n");

	while (n < totalSlice)
	{
		if (spine_vertex.find(n) == spine_vertex.end() || spine_vertex[n].size() < 3)
		{
			spine_vertex[n].assign(3, std::make_pair(0, 0));
			spine_vertex[n] = spine_vertex[n-1];
		}
		++n;
	}
	TRACE("Vertex Fix : Success ! \n");

	// 修正偏移太多的「中上點」
	n = 1;
	while (n < totalSlice)
	{
		if (spine_vertex.find(n) != spine_vertex.end())
		{
			if (abs(spine_vertex[n][0].first - spine_vertex[n - 1][0].first) > 10 ||
				abs(spine_vertex[n][0].second - spine_vertex[n - 1][0].second) > 10)
			{
				spine_vertex[n][0].first = spine_vertex[n - 1][0].first;
				spine_vertex[n][0].second = spine_vertex[n - 1][0].second;
			}
		}
		++n;
	}
	TRACE("Vertex Mid Fix : Success !! \n");

	// 尋找每張slice頂點(vertex)的垂直邊界
	auto findBorder = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			if (spine_vertex.find(s) == spine_vertex.end() || spine_vertex[s].size() < 3)
			{
				TRACE1("%3d slice without vertex !! \n", s);
				s += 2;
				continue;
			}
			
			spine_edge[s].assign(4, 0);
			spine_edge[s].at(0) = spine_vertex[s][1].first - 1;						// [0]: x_min
			spine_edge[s].at(1) = spine_vertex[s][2].first + 1;						// [1]: x_max
			spine_edge[s].at(2) = spine_vertex[s][0].second - 1;					// [2]: y_min
			spine_edge[s].at(3) = max(
				spine_vertex[s][1].second, spine_vertex[s][2].second) + 1;			// [3]: y_max
			s += 2;
		}
		if (start_slice == 0) TRACE("Find Border : Even Slice Success ! \n");
		else TRACE("Find Border : Odd Slice Success ! \n");
	};

	thread th_2(findBorder, 0);
	thread th_3(findBorder, 1);
	th_2.join(); th_3.join();

	// 計算每一張slice的斜線方程式係數(斜率.截距)
	auto lineIndex = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			/*if (spine_vertex.find(s) == spine_vertex.end() || spine_vertex[s].size() < 3)
			{
				TRACE1("%3d slice without vertex !! \n", s);
				s += 2;
				continue;
			}*/

			// 計算每張slice三角頂點的斜線方程式係數
			spine_line[s].assign(2, std::make_pair(0.0f, 0.0f));
			float slope1 = 0, slope2 = 0;						// 斜率 slope
			float inter1 = 0, inter2 = 0;						// 截距 intercept
			slope1 = (float)(spine_vertex[s][0].second - spine_vertex[s][1].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][1].first);
			slope2 = (float)(spine_vertex[s][0].second - spine_vertex[s][2].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][2].first);

			inter1 = (float)(spine_vertex[s][0].second + spine_vertex[s][1].second) -
				slope1 * (spine_vertex[s][0].first + spine_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(spine_vertex[s][0].second + spine_vertex[s][2].second) -
				slope2 * (spine_vertex[s][0].first + spine_vertex[s][2].first);
			inter2 /= 2;
			spine_line[s].at(0) = std::make_pair(slope1, inter1);	// 左線
			spine_line[s].at(1) = std::make_pair(slope2, inter2);	// 右線
			
			s += 2;
		}
		if (start_slice == 0) TRACE("Line Index : Even Slice Success ! \n");
		else TRACE("Line Index : Odd Slice Success ! \n");
	};

	thread th_4(lineIndex, 0);
	thread th_5(lineIndex, 1);
	th_4.join(); th_5.join();

	// Fix Spine Line
	for (int s = 0; s < totalSlice; ++s)
	{
		if (spine_line[s].size() < 2)
		{
			spine_line[s].clear();
			spine_line[s].assign(2, std::make_pair(0.0f, 0.0f));
			float slope1 = 0, slope2 = 0;						// 斜率 slope
			float inter1 = 0, inter2 = 0;						// 截距 intercept
			slope1 = (float)(spine_vertex[s][0].second - spine_vertex[s][1].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][1].first);
			slope2 = (float)(spine_vertex[s][0].second - spine_vertex[s][2].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][2].first);

			inter1 = (float)(spine_vertex[s][0].second + spine_vertex[s][1].second) -
				slope1 * (spine_vertex[s][0].first + spine_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(spine_vertex[s][0].second + spine_vertex[s][2].second) -
				slope2 * (spine_vertex[s][0].first + spine_vertex[s][2].first);
			inter2 /= 2;
			spine_line[s].at(0) = std::make_pair(slope1, inter1);	// 左線
			spine_line[s].at(1) = std::make_pair(slope2, inter2);	// 右線
		}
	}

	TRACE1("Spine Vertex's size : %3d.\n", spine_vertex.size());
	TRACE1("Spine Line's size : %3d.\n", spine_line.size());
	TRACE1("Spine Edge's size : %3d.\n", spine_edge.size());

	// 對每一張slice的垂直邊界範圍做pixel處理
	auto pixProcess = [&](int start_slice)
	{
		int s = start_slice;
		std::map<int, std::vector<int>>::iterator it;
		while (s < totalSlice)
		{
			if (spine_edge.find(s) == spine_edge.end() || spine_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found! \n", s);
				s += 2;
				continue;
			}

			it = spine_edge.find(s);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					if (pro[s][j * col + i] <= 100)
						pro[s][j * col + i] = 0;
					else if (pro[s][j * col + i] <= 140)
						pro[s][j * col + i] -= 30;
					//else if (pro[s][j * col + i] <= 200 && pro[s][j * col + i] > 180)
					//	pro[s][j * col + i] += 30;
				}
			}
			s += 2;
		}
		if (start_slice == 0) TRACE("Pixel Process : Even Slice Success ! \n");
		else TRACE("Pixel Process : Odd Slice Success ! \n");
	};

	/*thread th_6(pixProcess, 0);
	thread th_7(pixProcess, 1);
	th_6.join(); th_7.join();*/

	// 低通 濾波 (mean filter)
	//std::vector<int> avg_coef = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };
	std::vector<int> avg_coef = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	int avg_cnt = std::accumulate(avg_coef.begin(), avg_coef.end(), 0);

	auto outOfImg = [=](int px, int py)			// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0)
			return false;
		else return true;
	};
	auto avgKernel = [=](BYTE* img, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				if (!outOfImg(i, j))
				{
					sum += (avg_coef[n] * img[j * col + i]);
					n += 1;
				}
			}
		}
		return sum / avg_cnt;
	};
	auto medianKernel = [=](BYTE* img, int x, int y)
	{
		std::vector<BYTE> temp;
		temp.reserve(9);
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				if (!outOfImg(i, j))
				{
					temp.push_back(img[j * col + i]);
				}
			}
		}
		std::sort(temp.begin(), temp.end());
		auto len = temp.size();
		return temp[len / 2];
	};

	auto lowFilter = [&](int start_slice)
	{
		std::map<int, std::vector<int>>::iterator it;
		int s = start_slice;
		while (s < totalSlice)
		{
			if (spine_edge.find(s) == spine_edge.end() || spine_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found ! \n", s);
				s += 2;
				continue;
			}
			it = spine_edge.find(s);
			BYTE* tmp = new BYTE[row * col];
			std::memcpy(tmp, pro[s], sizeof(BYTE) * row * col);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					pro[s][j * col + i] = avgKernel(tmp, i, j);
					//pro[s][j * col + i] = medianKernel(tmp, i, j);
				}
			}
			delete[] tmp;
			s += 2;
		}
		if (start_slice == 0)	TRACE("Avg Filter : Even Slice Success ! \n");
		else TRACE("Avg Filter : Odd Slice Success ! \n");
	};

	thread th_8(lowFilter, 0);
	thread th_9(lowFilter, 1);
	th_8.join(); th_9.join();

	// 高通 濾波 (laplace filter)
	//std::vector<int> sharp_coef = {1, 1, 1, 1, -8, 1, 1, 1, 1};
	std::vector<int> laplace_coef = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
	const int weight = (laplace_coef[4] > 0) ? 1 : -1;

	auto laplaceKernel = [=](BYTE* img, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				if (!outOfImg(i, j))
				{
					sum += (laplace_coef[n] * img[j * col + i]);
					n += 1;
				}
			}
		}
		return sum;
	};
	auto laplaceFilter = [&](int start_slice)
	{
		std::map<int, std::vector<int>>::iterator it;
		int s = start_slice, pixel = 0;
		while (s < totalSlice)
		{
			if (spine_edge.find(s) == spine_edge.end() || spine_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found ! \n", s);
				s += 2;
				continue;
			}
			it = spine_edge.find(s);
			BYTE* tmp = new BYTE[row * col];
			std::memcpy(tmp, pro[s], sizeof(BYTE) * row * col);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					pixel = weight * laplaceKernel(tmp, i, j);
					pixel = tmp[j * col + i] + pixel;
					if (pixel > 255)	pixel = 255;
					else if (pixel < 0) pixel = 0;
					pro[s][j * col + i] = pixel;
				}
			}
			delete[] tmp;
			s += 2;
		}
		if (start_slice == 0)	TRACE("High Filter : Even Slice Success ! \n");
		else TRACE("High Filter : Odd Slice Success!\n");
	};

	thread th_10(laplaceFilter, 0);
	thread th_11(laplaceFilter, 1);
	th_10.join(); th_11.join();
	
}

void C3DProcess::Spine_process_fix()
{
	// 脊椎進行二次成長後，重新取頂點、限制線、邊界
	//
	const int obj1 = 1;
	const int obj2 = 2;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	BYTE**& org = m_pDoc->m_img;
	BYTE**& pro = m_pDoc->m_imgPro;

	// 三角頂點
	// 尋找每張slice的三角頂點
	auto findVertex = [&](int start_slice)
	{
		int pos, x, y;
		int s = start_slice;
		std::vector<int> x_pos;
		std::vector<int> y_pos;
		x_pos.reserve(15000);
		y_pos.reserve(15000);

		while (s < totalSlice)
		{
			pos = 0;
			int ver_mid = 0, y_min = 512;
			int ver_lft = 0, ver_rht = 0;

			// 紀錄每個種子點的x.y座標值
			for (x = 0; x < col; ++x)
			{
				for (y = 0; y < row; ++y)
				{
					pos = y * col + x;
					if (judge[s][pos] == obj1 ||
						judge[s][pos] == obj2)
					{
						x_pos.push_back(x);
						y_pos.push_back(y);

						// 尋找-中上點(中間最高)
						if (x < 350 && y < y_min)
						{
							y_min = y;
							ver_mid = pos;
						}
						// 尋找-左下點(第一個接觸到的點)
						if (ver_lft == 0)
							ver_lft = pos;
					}
				}
			}

			// 如果成長範圍太少導致判別會錯誤，就先跳過
			if (x_pos.size() < 10 || y_pos.size() < 10)
			{
				TRACE1("Slice : %3d cannot find vertex. \n", s);
				x_pos.clear();
				y_pos.clear();
				s += 2;
				continue;
			}
			size_t x_len = x_pos.size();
			size_t y_len = y_pos.size();

			// 尋找-右下點(該行最先接觸到的點)
			//ver_rht = y_pos.at(y_len - 1) * col + x_pos.at(x_len - 1);
			x = x_pos.at(x_len - 1); y = 0;
			while (y < row)
			{
				if (judge[s][y * col + x] == obj1 ||
					judge[s][y * col + x] == obj2)
					break;
				y++;
			}
			ver_rht = y * col + x;

			// 存三角頂點 - 中上點
			spine_vertex[s].at(0) = std::make_pair((ver_mid % col), (ver_mid / col));
			// 存三角頂點 - 左下點
			spine_vertex[s].at(1) = std::make_pair((ver_lft % col), (ver_lft / col));
			// 計算三角頂點 - 右下點 (避免成長到影像一半以上的肋骨)
			if ((ver_rht / col) <= (ver_mid / col) || (ver_rht % col) - (ver_mid % col) >= 200)
			{
				x = (ver_lft % col) + 2 * ((ver_mid % col) - (ver_lft % col));
				y = ver_lft / col;
				spine_vertex[s].at(2) = std::make_pair(x, y);
			}
			else
				spine_vertex[s].at(2) = std::make_pair((ver_rht % col), (ver_rht / col));

			x_pos.clear();
			y_pos.clear();
			s += 2;
		}	// end while

		x_pos.shrink_to_fit();
		y_pos.shrink_to_fit();
		if (start_slice == 0)	TRACE("Find Vertex : Even Slice Success ! \n");
		else TRACE("Find Vertex : Odd Slice Success ! \n");
	};	// end findVertex()

	thread th_0(findVertex, 0);
	thread th_1(findVertex, 1);
	th_0.join(); th_1.join();

	// 尋找每張slice頂點(vertex)的垂直邊界
	auto findBorder = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			if (spine_vertex.find(s) == spine_vertex.end() || spine_vertex[s].size() < 3)
			{
				TRACE1("%3d slice without vertex !! \n", s);
				s += 2;
				continue;
			}

			spine_edge[s].at(0) = spine_vertex[s][1].first - 1;						// [0]: x_min
			spine_edge[s].at(1) = spine_vertex[s][2].first + 1;						// [1]: x_max
			spine_edge[s].at(2) = spine_vertex[s][0].second - 1;					// [2]: y_min
			spine_edge[s].at(3) = max(
				spine_vertex[s][1].second, spine_vertex[s][2].second) + 1;			// [3]: y_max
			s += 2;
		}
		if (start_slice == 0) TRACE("Find Border : Even Slice Success ! \n");
		else TRACE("Find Border : Odd Slice Success ! \n");
	};

	thread th_2(findBorder, 0);
	thread th_3(findBorder, 1);
	th_2.join(); th_3.join();

	// 計算每一張slice的斜線方程式係數(斜率.截距)
	auto lineIndex = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			if (spine_vertex.find(s) == spine_vertex.end() || spine_vertex[s].size() < 3)
			{
				TRACE1("%3d slice without vertex !! \n", s);
				s += 2;
				continue;
			}

			// 計算每張slice三角頂點的斜線方程式係數
			float slope1 = 0, slope2 = 0;						// 斜率 slope
			float inter1 = 0, inter2 = 0;						// 截距 intercept
			slope1 = (float)(spine_vertex[s][0].second - spine_vertex[s][1].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][1].first);
			slope2 = (float)(spine_vertex[s][0].second - spine_vertex[s][2].second) /
				(float)(spine_vertex[s][0].first - spine_vertex[s][2].first);

			inter1 = (float)(spine_vertex[s][0].second + spine_vertex[s][1].second) -
				slope1 * (spine_vertex[s][0].first + spine_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(spine_vertex[s][0].second + spine_vertex[s][2].second) -
				slope2 * (spine_vertex[s][0].first + spine_vertex[s][2].first);
			inter2 /= 2;
			spine_line[s].at(0) = std::make_pair(slope1, inter1);	// 左線
			spine_line[s].at(1) = std::make_pair(slope2, inter2);	// 右線

			s += 2;
		}
		if (start_slice == 0) TRACE("Line Index : Even Slice Success ! \n");
		else TRACE("Line Index : Odd Slice Success ! \n");
	};

	thread th_4(lineIndex, 0);
	thread th_5(lineIndex, 1);
	th_4.join(); th_5.join();

	TRACE1("Spine line's size : %d. \n", spine_line.size());
	TRACE1("Spine edge's size : %d. \n", spine_edge.size());
	TRACE1("Spine vertex's size : %d. \n", spine_vertex.size());

}

void C3DProcess::Sternum_process()
{
	// 胸骨進行二次成長前的預處理
	//
	const int obj = 3;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	BYTE**& org = m_pDoc->m_img;
	BYTE**& pro = m_pDoc->m_imgPro;

	// 尋找每張slice的三角頂點
	auto findVertex = [&](int start_slice)
	{
		int pos, x, y;
		int s = start_slice;
		std::vector<int> x_pos;
		std::vector<int> y_pos;
		x_pos.reserve(20000);
		y_pos.reserve(20000);

		while (s < totalSlice)
		{
			pos = 0;
			int ly_max = 0, ry_max = 0;
			int ver_lft = 0, ver_rht = 0, ver_mid = 0;

			// 紀錄每個種子點的x.y座標值
			for (x = 0; x < col; ++x)
			{
				for (y = (row - 1); y >= 0; --y)
				{
					pos = y * col + x;
					if (judge[s][pos] == obj)
					{
						x_pos.push_back(x);
						y_pos.push_back(y);

						// 先大概搜尋左右邊最低(y最高)的點
						// 不一定是要用來做限制線的點位置
						if (x < 256 && ver_lft == 0)
						{
							ver_lft = pos;
						}
						else if (x > 256 && y > ry_max)
						{
							ry_max = y;
							ver_rht = pos;
						}
					}
				}
			}

			// 如果成長範圍太少導致判別會錯誤，就先跳過
			if (x_pos.size() < 10 || y_pos.size() < 10)
			{
				//TRACE1("Slice : %3d cannot find vertex. \n", s);
				x_pos.clear();
				y_pos.clear();
				s += 2;
				continue;
			}

			// 搜尋中間點
			int mx = ((ver_rht % col) + (ver_lft % col)) / 2;
			while (ver_mid == 0)
			{
				for (int y = (row - 1); y >= 0; --y)
				{
					pos = y * col + mx;
					if (judge[s][pos] == obj)
					{
						ver_mid = pos;
						break;
					}
				}
				mx -= 1;
			}
			
			size_t x_len = x_pos.size();
			size_t y_len = y_pos.size();

			// 存頂點 - 初始化三個數值(避免 subscript out of range)
			sternum_vertex[s].assign(3, std::make_pair(0, 0));
			// 存三角頂點 - 中上點
			sternum_vertex[s].at(0) = std::make_pair((ver_mid % col), (ver_mid / col));
			// 存三角頂點 - 左下點
			sternum_vertex[s].at(1) = std::make_pair((ver_lft % col), (ver_lft / col));
			// 存三角頂點 - 右下點
			sternum_vertex[s].at(2) = std::make_pair((ver_rht % col), (ver_rht / col));

			// 取範圍邊界
			auto x_mm = std::minmax_element(x_pos.begin(), x_pos.end());
			auto y_mm = std::minmax_element(y_pos.begin(), y_pos.end());

			// 存邊界位置 - 初始化
			sternum_edge[s].assign(4, 0);
			sternum_edge[s].at(0) = *(x_mm.first);		// 存邊界 - x 最小值
			sternum_edge[s].at(1) = *(x_mm.second);		// 存邊界 - x 最大值
			sternum_edge[s].at(2) = *(y_mm.first);		// 存邊界 - y 最小值
			sternum_edge[s].at(3) = *(y_mm.second);		// 存邊界 - y 最大值

			x_pos.clear();
			y_pos.clear();
			s += 2;
		}	// end while

		x_pos.shrink_to_fit();
		y_pos.shrink_to_fit();
		if (start_slice == 0)	TRACE("Find Vertex : Even Slice Success ! \n");
		else TRACE("Find Vertex : Odd Slice Success ! \n");
	};	// end findVertex();

	thread th_0(findVertex, 0);
	thread th_1(findVertex, 1);
	th_0.join();	th_1.join();

	 // 修正沒有判斷到頂點的slice
	int n = 0;
	if (sternum_vertex.find(n) == sternum_vertex.end() || sternum_vertex[n].size() < 3)
	{
		int s = n + 1;
		// 直到找到有頂點的slice為止
		while (sternum_vertex.find(s) == sternum_vertex.end() || sternum_vertex[s].size() < 3)
			s++;
		for (; n < s; ++n)
		{
			sternum_vertex[n].assign(3, std::make_pair(0, 0));
			sternum_vertex[n] = sternum_vertex[s];
		}
		TRACE("0 slice's vertex have been fixed ! \n");
	}
	else
		TRACE("0 slice have vertex ! \n");

	while (n < totalSlice)
	{
		if (sternum_vertex.find(n) == sternum_vertex.end() || sternum_vertex[n].size() < 3)
		{
			sternum_vertex[n].assign(3, std::make_pair(0, 0));
			sternum_vertex[n] = sternum_vertex[n - 1];
		}
		++n;
	}
	TRACE("Vertex Fix : Success ! \n");

	// 修正沒有判斷到邊界的slice
	n = 0;
	if (sternum_edge.find(n) == sternum_edge.end() || sternum_edge[n].size() < 4)
	{
		int s = n + 1;
		while (sternum_edge.find(s) == sternum_edge.end() || sternum_edge[s].size() < 4)
			s++;
		for (; n < s; ++n)
		{
			sternum_edge[n].assign(4, 0);
			sternum_edge[n] = sternum_edge[s];
		}
		TRACE("0 slice's edge have been fixed ! \n");
	}
	else
		TRACE("0 slice have edge ! \n");

	while (n < totalSlice)
	{
		if (sternum_edge.find(n) == sternum_edge.end() || sternum_edge[n].size() < 4)
		{
			sternum_edge[n].assign(4, 0);
			sternum_edge[n] = sternum_edge[n - 1];
		}
		++n;
	}
	TRACE("Edge Fix : Success ! \n");

	// 計算斜線方程式的係數
	auto lineIndex = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			if (sternum_vertex.find(s) == sternum_vertex.end())
			{
				TRACE1("%3d slice vertex not found! \n", s);
				s += 2;
				continue;
			}

			// 存直線方程式係數 - 初始化
			sternum_line[s].assign(2, std::make_pair(0.0f, 0.0f));

			float slope1 = 0, slope2 = 0;		// 斜率 slope
			float inter1 = 0, inter2 = 0;		// 截距 intercept

			slope1 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][1].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][1].first);
			slope2 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][2].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][2].first);

			inter1 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][1].second) -
				slope1 * (sternum_vertex[s][0].first + sternum_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][2].second) -
				slope2 * (sternum_vertex[s][0].first + sternum_vertex[s][2].first);
			inter2 /= 2;

			sternum_line[s].at(0) = std::make_pair(slope1, inter1);		// 左線
			sternum_line[s].at(1) = std::make_pair(slope2, inter2);		// 右線

			s += 2;
		}
		if (start_slice == 0) TRACE("Line Index : Even Slice Success ! \n");
		else TRACE("Line Index : Odd Slice Success ! \n");
	};

	thread th_2(lineIndex, 0);
	thread th_3(lineIndex, 1);
	th_2.join();	th_3.join();

	// 修正沒有計算出sternum_line的slice
	for (int s = 0; s < totalSlice; ++s)
	{
		if (sternum_line[s].size() < 2)
		{
			sternum_line[s].clear();
			sternum_line[s].assign(2, std::make_pair(0.0f, 0.0f));

			float slope1 = 0, slope2 = 0;		// 斜率 slope
			float inter1 = 0, inter2 = 0;		// 截距 intercept

			slope1 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][1].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][1].first);
			slope2 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][2].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][2].first);

			inter1 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][1].second) -
				slope1 * (sternum_vertex[s][0].first + sternum_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][2].second) -
				slope2 * (sternum_vertex[s][0].first + sternum_vertex[s][2].first);
			inter2 /= 2;

			sternum_line[s].at(0) = std::make_pair(slope1, inter1);		// 左線
			sternum_line[s].at(1) = std::make_pair(slope2, inter2);		// 右線
		}
	}
	TRACE("Sternum line Fix : Success ! \n");

	TRACE1("Sternum line's size : %d. \n", sternum_line.size());
	TRACE1("Sternum edge's size : %d. \n", sternum_edge.size());
	TRACE1("Sternum vertex's size : %d. \n", sternum_vertex.size());

	// 對每一張slice的垂直邊界範圍作pixel處理
	auto pixProcess = [&](int start_slice)
	{
		int s = start_slice;
		std::map<int, std::vector<int>>::iterator it;
		while (s < totalSlice)
		{
			if (sternum_edge.find(s) == sternum_edge.end() || sternum_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found! \n", s);
				s += 2;
				continue;
			}

			it = sternum_edge.find(s);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					if (pro[s][j * col + i] <= 100)
						pro[s][j * col + i] = 0;
					else if (pro[s][j * col + i] <= 140)
						pro[s][j * col + i] -= 30;
					//else if (pro[s][j * col + i] <= 200 && pro[s][j * col + i] > 180)
					//	pro[s][j * col + i] += 30;
				}
			}
			s += 2;
		}
		if (start_slice == 0) TRACE("Pixel Process : Even Slice Success ! \n");
		else TRACE("Pixel Process : Odd Slice Success ! \n");
	};

	thread th_4(pixProcess, 0);
	thread th_5(pixProcess, 1);
	th_4.join();	th_5.join();

	// 低通 濾波 (mean filter)
	//std::vector<int> avg_coef = { 1, 2, 1, 2, 4, 2, 1, 2, 1 };
	std::vector<int> avg_coef = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	int avg_cnt = std::accumulate(avg_coef.begin(), avg_coef.end(), 0);

	auto outOfImg = [=](int px, int py)			// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0)
			return false;
		else return true;
	};

	auto avgKernel = [=](BYTE* img, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				if (!outOfImg(i, j))
				{
					sum += (avg_coef[n] * img[j * col + i]);
					n += 1;
				}
			}
		}
		return sum / avg_cnt;
	};
	auto lowFilter = [&](int start_slice)
	{
		std::map<int, std::vector<int>>::iterator it;
		int s = start_slice;
		while (s < totalSlice)
		{
			if (sternum_edge.find(s) == sternum_edge.end() || sternum_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found ! \n", s);
				s += 2;
				continue;
			}
			it = sternum_edge.find(s);
			BYTE* tmp = new BYTE[row * col];
			std::memcpy(tmp, pro[s], sizeof(BYTE) * row * col);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					pro[s][j * col + i] = avgKernel(tmp, i, j);
				}
			}
			delete[] tmp;
			s += 2;
		}
		if (start_slice == 0)	TRACE("Avg Filter : Even Slice Success ! \n");
		else TRACE("Avg Filter : Odd Slice Success ! \n");
	};

	thread th_6(lowFilter, 0);
	thread th_7(lowFilter, 1);
	th_6.join();	th_7.join();

	// 高通 濾波 (laplace filter)
	//std::vector<int> sharp_coef = {1, 1, 1, 1, -8, 1, 1, 1, 1};
	std::vector<int> laplace_coef = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
	const int weight = (laplace_coef[4] > 0) ? 1 : -1;

	auto laplaceKernel = [=](BYTE* img, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				if (!outOfImg(i, j))
				{
					sum += (laplace_coef[n] * img[j * col + i]);
					n += 1;
				}
			}
		}
		return sum;
	};

	auto laplaceFilter = [&](int start_slice)
	{
		std::map<int, std::vector<int>>::iterator it;
		int s = start_slice, pixel = 0;
		while (s < totalSlice)
		{
			if (sternum_edge.find(s) == sternum_edge.end() || sternum_edge[s].size() < 4)
			{
				TRACE1("%3d slice edge not found ! \n", s);
				s += 2;
				continue;
			}
			it = sternum_edge.find(s);
			BYTE* tmp = new BYTE[row * col];
			std::memcpy(tmp, pro[s], sizeof(BYTE) * row * col);
			for (int j = it->second.at(2); j <= it->second.at(3); ++j)
			{
				for (int i = it->second.at(0); i <= it->second.at(1); ++i)
				{
					pixel = weight * laplaceKernel(tmp, i, j);
					pixel = tmp[j * col + i] + pixel;
					if (pixel > 255)	pixel = 255;
					else if (pixel < 0) pixel = 0;
					pro[s][j * col + i] = pixel;
				}
			}
			delete[] tmp;
			s += 2;
		}
		if (start_slice == 0)	TRACE("High Filter : Even Slice Success ! \n");
		else TRACE("High Filter : Odd Slice Success!\n");
	};

	thread th_8(laplaceFilter, 0);
	thread th_9(laplaceFilter, 1);
	th_8.join();	th_9.join();

}

void C3DProcess::Sternum_process_fix()
{
	// 胸骨二次成長後，重新取頂點、邊界、限制線
	//
	const int obj1 = 3;
	const int obj2 = 4;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	BYTE**& org = m_pDoc->m_img;
	BYTE**& pro = m_pDoc->m_imgPro;

	// 尋找每張slice的三角頂點
	auto findVertex = [&](int start_slice)
	{
		int pos, x, y;
		int s = start_slice;
		std::vector<int> x_pos;
		std::vector<int> y_pos;
		x_pos.reserve(10000);
		y_pos.reserve(10000);

		while (s < totalSlice)
		{
			pos = 0;
			int ly_max = 0, ry_max = 0;
			int ver_lft = 0, ver_rht = 0, ver_mid = 0;

			// 紀錄每個種子點的x.y座標值
			for (x = 0; x < col; ++x)
			{
				for (y = (row - 1); y >= 0; --y)
				{
					pos = y * col + x;
					if (judge[s][pos] == obj1 ||
						judge[s][pos] == obj2)
					{
						x_pos.push_back(x);
						y_pos.push_back(y);

						// 先大概搜尋左右邊最低(y最高)的點
						// 不一定是要用來做限制線的點位置
						if (x < 256 && ver_lft == 0)
						{
							//ly_max = y;
							ver_lft = pos;
						}
						else if (x > 256 && y > ry_max)
						{
							ry_max = y;
							ver_rht = pos;
						}
					}
				}
			}

			// 如果成長範圍太少導致判別會錯誤，就先跳過
			if (x_pos.size() < 10 || y_pos.size() < 10)
			{
				//TRACE1("Slice : %3d cannot find vertex. \n", s);
				x_pos.clear();
				y_pos.clear();
				s += 2;
				continue;
			}

			// 搜尋中間點
			int mx = ((ver_rht % col) + (ver_lft % col)) / 2;
			while (ver_mid == 0)
			{
				for (int y = (row - 1); y >= 0; --y)
				{
					pos = y * col + mx;
					if (judge[s][pos] == obj1 ||
						judge[s][pos] == obj2)
					{
						ver_mid = pos;
						break;
					}
				}
				mx -= 1;
			}
			
			size_t x_len = x_pos.size();
			size_t y_len = y_pos.size();

			// 存三角頂點 - 中上點
			sternum_vertex[s].at(0) = std::make_pair((ver_mid % col), (ver_mid / col));
			// 存三角頂點 - 左下點
			sternum_vertex[s].at(1) = std::make_pair((ver_lft % col), (ver_lft / col));
			// 存三角頂點 - 右下點
			sternum_vertex[s].at(2) = std::make_pair((ver_rht % col), (ver_rht / col));

			// 取範圍邊界
			auto x_mm = std::minmax_element(x_pos.begin(), x_pos.end());
			auto y_mm = std::minmax_element(y_pos.begin(), y_pos.end());

			// 存邊界位置
			sternum_edge[s].at(0) = *(x_mm.first);		// 存邊界 - x 最小值
			sternum_edge[s].at(1) = *(x_mm.second);		// 存邊界 - x 最大值
			sternum_edge[s].at(2) = *(y_mm.first);		// 存邊界 - y 最小值
			sternum_edge[s].at(3) = *(y_mm.second);		// 存邊界 - y 最大值

			x_pos.clear();
			y_pos.clear();
			s += 2;
		}	// end while

		x_pos.shrink_to_fit();
		y_pos.shrink_to_fit();
		if (start_slice == 0)	TRACE("Find Vertex : Even Slice Success ! \n");
		else TRACE("Find Vertex : Odd Slice Success ! \n");
	};	// end findVertex();

	thread th_0(findVertex, 0);
	thread th_1(findVertex, 1);
	th_0.join();	th_1.join();

	auto lineIndex = [&](int start_slice)
	{
		int s = start_slice;
		while (s < totalSlice)
		{
			if (sternum_vertex.find(s) == sternum_vertex.end())
			{
				TRACE1("%3d slice vertex not found! \n", s);
				s += 2;
				continue;
			}

			// 存直線方程式係數
			float slope1 = 0, slope2 = 0;		// 斜率 slope
			float inter1 = 0, inter2 = 0;		// 截距 intercept

			slope1 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][1].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][1].first);
			slope2 = (float)(sternum_vertex[s][0].second - sternum_vertex[s][2].second) /
				(float)(sternum_vertex[s][0].first - sternum_vertex[s][2].first);

			inter1 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][1].second) -
				slope1 * (sternum_vertex[s][0].first + sternum_vertex[s][1].first);
			inter1 /= 2;
			inter2 = (float)(sternum_vertex[s][0].second + sternum_vertex[s][2].second) -
				slope2 * (sternum_vertex[s][0].first + sternum_vertex[s][2].first);
			inter2 /= 2;

			sternum_line[s].at(0) = std::make_pair(slope1, inter1);		// 左線
			sternum_line[s].at(1) = std::make_pair(slope2, inter2);		// 右線

			s += 2;
		}
		if (start_slice == 0) TRACE("Line Index : Even Slice Success ! \n");
		else TRACE("Line Index : Odd Slice Success ! \n");
	};

	thread th_2(lineIndex, 0);
	thread th_3(lineIndex, 1);
	th_2.join();	th_3.join();



}

void C3DProcess::OnBnClickedButtonGrowingRemove()
{
	// TODO: Add your control notification handler code here
	// Button : Growing Remove
	// 把骨骼的部分(包括在這之間的肌肉)變透明
	//
	if (!get_regionGrow)	return;

	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;
	register int i, j, k;
	int obj1 = 1, obj2 = 2;
	int obj3 = 3, obj4 = 4;

	// 脊椎骨的部分
	if (m_spine)
	{
		int long_slice = 0;
		int long_dis = spine_vertex[0][2].first - spine_vertex[0][1].first;
		int z_temp;

		k = 0;
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				int z_cur = k - (Mat_Offset + 1);		// 現在的切片數
				int cur_dis = spine_vertex[z_cur][2].first - spine_vertex[z_cur][1].first;

				if (abs(cur_dis - long_dis) > 30)
				{
					z_temp = long_slice;
				}
				else
				{
					z_temp = z_cur;
					long_dis = cur_dis;
					long_slice = z_cur;
				}

				// 先大概消除
				for (i = 2; i < col - 2; i += 1)
				{
					int y = 0;
					while (y < row && (judge[z_cur][y * col + i] == 0 ||
						judge[z_cur][y * col + i] == obj3 || 
						judge[z_cur][y * col + i] == obj4 || 
						judge[z_cur][y * col + i] == -obj3 || 
						judge[z_cur][y * col + i] == -obj4 ))
						y++;
					for (j = y; j < row - 2; j += 1)
					{
						for (int nk = -3; nk <= 3; ++nk)
						{
							for (int nj = -2; nj <= 2; ++nj)
							{
								for (int ni = -2; ni <= 2; ++ni)
								{
									getRamp(&m_image0[((i + ni) / 2) * 256 * 256 + ((j + nj) / 2) * 256 + ((k + nk) / 2)][0],
										0, 0);
								}
							}
						}
					}

					for (j = 2; j < row - 2; j += 2)
					{
						if (judge[z_cur][j * col + i] == obj1 || judge[z_cur][j * col + i] == obj2)
							getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								0, 0);
					}
				}

				// 剩下的肌肉層
				int x_begin = spine_vertex[z_temp][1].first;
				int x_end = spine_vertex[z_temp][2].first;
				int y_begin = min(spine_vertex[z_temp][1].second, spine_vertex[z_temp][2].second);
				int y_end = row;

				for (j = y_begin; j < y_end; ++j)
				{
					for (i = x_begin; i < x_end; ++i)
					{
						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							0, 0);
					}
				}

			}
			k += 2;
		}
	}
	// 胸骨的部分
	else if (m_sternum)
	{
		int long_slice = 0;
		int long_dis = sternum_vertex[0][2].first - sternum_vertex[0][1].first;
		int y_mid_pre = sternum_vertex[0][0].second;

		k = 0;
		while (k < 512)
		{
			if (k > sample_start && k <= sample_end)
			{
				int z_temp;
				int z_cur = k - (Mat_Offset + 1);
				int cur_dis = sternum_vertex[z_cur][2].first - sternum_vertex[z_cur][1].first;

				if (abs(cur_dis - long_dis) > 30)
				{
					z_temp = long_slice;
				}
				else
				{
					z_temp = z_cur;
					long_dis = cur_dis;
					long_slice = z_cur;
				}
				
				int y = 0; 							// 每一行 x 要開始消除的起始 y
				int	y_pre = row - 1;				// 前一行 x 要開始消除的起始 y
				for (int i = 2; i < (col - 2); ++i)
				{
					// 先大概消除不要的部分
					y = row - 1;
					while (y >= 0 && (judge[z_cur][y * col + i] == 0 ||
						judge[z_cur][y * col + i] == obj1 ||
						judge[z_cur][y * col + i] == obj2 ||
						judge[z_cur][y * col + i] == -obj1 ||
						judge[z_cur][y * col + i] == -obj2))
						--y;

					if (y == row - 1)
						y = y_pre;
					else
						y_pre = y;
					
					for (j = y; j >= 2; --j)
					{
						for (int nk = -1; nk <= 1; ++nk)
						{
							for (int nj = -1; nj <= 1; ++nj)
							{
								for (int ni = -1; ni <= 1; ++ni)
								{
									getRamp(&m_image0[((i + ni) / 2) * 256 * 256 + ((j + nj) / 2) * 256 + ((k + nk) / 2)][0],
										0, 0);
								}
							}
						}
					}
					y_pre = y;

					int y_mid = INT_MAX;
					if (!get_mid_fix)
					{
						int y_min = 512;
						for (int i = 180; i <= 310; ++i)
						{
							for (int j = row - 1; j >= 0; --j)
							{
								if (judge[z_cur][j * col + i] == obj3 ||
									judge[z_cur][j * col + i] == obj4)
								{
									if (j < y_min) y_min = j;
									break;
								}
							}
						}
						y_mid = y_min;

						if (abs(y_mid - y_mid_pre) > 5)
							y_mid = y_mid_pre;
						else
							y_mid_pre = y_mid;
					}
					else
					{
						int x_mid = mid_fix_pt.first;
						for (int j = row-1; j >= 0; --j)
						{
							if (judge[z_cur][j * col + x_mid] == obj3 ||
								judge[z_cur][j * col + x_mid] == obj4)
							{
								y_mid = j;
								break;
							}
						}
						if (y_mid == INT_MAX || abs(y_mid - y_mid_pre) > 10)
						{
							if (z_cur <= 50) y_mid_pre = y_mid;
							else if (z_cur > 150) y_mid = mid_fix_pt.second;
							else y_mid = y_mid_pre;
						}
						else
							y_mid_pre = y_mid;
					}


					for (j = 2; j < row - 2; j += 2)
					{
						if (judge[z_cur][j * col + i] == obj3 ||
							judge[z_cur][j * col + i] == obj4 ||
							j <= y_mid)
							getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
								0, 0);
					}

				}	// end for

			}	// end if
			k += 2;
		}	// end while

	}	// end else if

	LoadVolume();
	Draw3DImage(true);
}

void C3DProcess::OnBnClickedButtonGrowingCapture()
{
	// TODO: Add your control notification handler code here
	// Button : Growing Capture
	//
	if (!get_regionGrow) return;

	const int row = ROW;
	const int col = COL;
	const int total_slice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + total_slice;
	register int i, j, k;
	int obj1 = 1, obj2 = 2;
	int obj3 = 3, obj4 = 4;
	float pixel;

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
							pixel / 255.0F, 0);
					}
					else
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
					else
					{
						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							pixel / 255.0F, 0);
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
	get_regionGrow = false;

	int obj1, obj2;
	if (m_spine)
	{
		get_spine = false;
		spine_volume = 0.0;
		obj1 = 1; obj2 = 2;
	}
	else if (m_sternum)
	{
		get_sternum = false;
		sternum_volume = 0.0;
		obj1 = 3; obj2 = 4;
	}

	GetDlgItem(IDC_BUTTON_DILATION)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_REMOVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_RECOVERY)->EnableWindow(FALSE);
	//------------------------------------------------------------//

	float pixel;
	register int i, j, k;
	const int row = ROW;
	const int col = COL;
	const int totalx = ROW * COL;
	const int totaly = Total_Slice;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;

	// 恢復 : 3D影像
	k = 0;
	while (k < 512)
	{
		if (k > sample_start && k <= sample_end)
		{
			int z = k - (Mat_Offset + 1);
			for (j = 2; j < row - 2; j += 2)
			{
				for (i = 2; i < col - 2; i += 2)
				{
					if (judge[z][j * col + i] == obj1 || judge[z][j * col + i] == -obj1 ||
						judge[z][j * col + i] == obj2 || judge[z][j * col + i] == -obj2)
					{
						pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * col + i];
						getRamp(&m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)][0],
							pixel / 255.0F, 0);
					}
				}
			}
		}
		k += 2;
	}
	
	// 恢復 : 成長判定矩陣 and 2D影像
	k = 0;
	while (k < totalSlice)
	{
		for (j = 0; j < row; ++j)
		{
			for (i = 0; i < col; ++i)
			{
				if (judge[k][j * col + i] == obj1 || judge[k][j * col + i] == -obj1 ||
					judge[k][j * col + i] == obj2 || judge[k][j * col + i] == -obj2)
				{
					judge[k][j * col + i] = 0;
				}
				pixel = m_pDoc->m_img[k][j * col + i];
				m_pDoc->m_imgPro[k][j * col + i] = (BYTE)pixel;
			}
		}
		k += 1;
	}

	LoadVolume();
	UpdateData(FALSE);
	Draw3DImage(true);
	Draw2DImage(DisplaySlice);
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
		glColor4f(0.902f, 1.0f, 1.0f, 0.1f);

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
				if (m_pDoc->m_img[slice][i] >= pixel_down_threshold && 
					m_pDoc->m_img[slice][i] <= pixel_up_threshold )
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
				if (m_pDoc->m_HUimg[slice][i] >= HU_down_threshold &&
					m_pDoc->m_HUimg[slice][i] <= HU_up_threshold )
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
				if (m_pDoc->m_imgPro[slice][i] >= pixel_down_threshold &&
					m_pDoc->m_imgPro[slice][i] <= pixel_up_threshold )
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
				if (m_pDoc->m_HUimg[slice][i] >= HU_down_threshold &&
					m_pDoc->m_HUimg[slice][i] <= HU_up_threshold )
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

	// 於2D影像顯示繪製的線(用於驗證)
	if (m_2Dverify)
	{
		if (!draw_spine_pt.empty())
		{
			std::map<int, std::vector<std::pair<int, int>>>::iterator it;
			if ((it = draw_spine_pt.find(slice)) != draw_spine_pt.end())
			{
				CPoint pt;
				for (const auto& n : it->second)
				{
					for (int j = -1; j <= 1; ++j)
					{
						for (int i = -1; i <= 1; ++i)
						{
							pt.x = n.first + i;
							pt.y = n.second + j;
							dc.SetPixel(pt, RGB(0, 255, 255));
						}
					}
				}
			}
		}
		if (!draw_sternum_pt.empty())
		{
			std::map<int, std::vector<std::pair<int, int>>>::iterator it;
			if ((it = draw_sternum_pt.find(slice)) != draw_sternum_pt.end())
			{
				CPoint pt;
				for (const auto& n : it->second)
				{
					for (int j = -1; j <= 1; ++j)
					{
						for (int i = -1; i <= 1; ++i)
						{
							pt.x = n.first + i;
							pt.y = n.second + j;
							dc.SetPixel(pt, RGB(255, 255, 0));
						}
					}
				}
			}
		}
		if (!draw_spine_line.empty())
		{
			std::map<int, std::set<std::pair<int, int>>>::iterator it;
			if ((it = draw_spine_line.find(slice)) != draw_spine_line.end())
			{
				CPoint pt;
				for (const auto& n : it->second)
				{
					for (int j = -1; j <= 1; ++j)
					{
						for (int i = -1; i <= 1; ++i)
						{
							pt.x = n.first + i;
							pt.y = n.second + j;
							dc.SetPixel(pt, RGB(0, 255, 255));
						}
					}
				}
			}
		}
		if (!draw_sternum_line.empty())
		{
			std::map<int, std::set<std::pair<int, int>>>::iterator it;
			if ((it = draw_sternum_line.find(slice)) != draw_sternum_line.end())
			{
				CPoint pt;
				for (const auto& n : it->second)
				{
					for (int j = -1; j <= 1; ++j)
					{
						for (int i = -1; i <= 1; ++i)
						{
							pt.x = n.first + i;
							pt.y = n.second + j;
							dc.SetPixel(pt, RGB(255, 255, 0));
						}
					}
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
			int obj1 = INT_MAX, obj2 = INT_MAX;
			int obj3 = INT_MAX, obj4 = INT_MAX;
			if (get_spine)
			{
				obj1 = 1;
				obj2 = 2;
			}
			if (get_sternum)
			{
				obj3 = 3;
				obj4 = 4;
			}
			for (j = 0; j < 512; j++)
			{
				for (i = 0; i < 512; i++)
				{
					if (judge[DisplaySlice][j * Col + i] == obj1 || 
						judge[DisplaySlice][j * Col + i] == obj3)
					{
						pt.x = i;
						pt.y = j;
						dc.SetPixel(pt, RGB(255, 120, 190));
					}
					else if (judge[DisplaySlice][j * Col + i] == obj2 ||
							judge[DisplaySlice][j * Col + i] == obj4)
					{
						pt.x = i;
						pt.y = j;
						dc.SetPixel(pt, RGB(100, 255, 100));
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
	if (!spine_vertex.empty())
	{
		std::map<int, std::vector<std::pair<int, int>>>::iterator it;
		it = spine_vertex.find(slice);
		CPoint pt;
	
		// 中上點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(0).first + i;
				pt.y = (LONG)it->second.at(0).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
			}
		}

		// 左下點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(1).first + i;
				pt.y = (LONG)it->second.at(1).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
			}
		}

		// 右下點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(2).first + i;
				pt.y = (LONG)it->second.at(2).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
			}
		}
	}
	if (!sternum_vertex.empty())
	{
		std::map<int, std::vector<std::pair<int, int>>>::iterator it;
		it = sternum_vertex.find(slice);
		CPoint pt;

		// 中上點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(0).first + i;
				pt.y = (LONG)it->second.at(0).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
			}
		}

		// 左下點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(1).first + i;
				pt.y = (LONG)it->second.at(1).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
			}
		}

		// 右下點
		for (i = -3; i <= 3; i++)
		{
			for (j = -3; j <= 3; j++)
			{
				pt.x = (LONG)it->second.at(2).first + i;
				pt.y = (LONG)it->second.at(2).second + j;

				dc.SetPixel(pt, RGB(255, 30, 30));
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
	int obj;
	if (m_spine) obj = 1;
	else if (m_sternum) obj = 3;
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int range = (factor.s_kernel - 1) / 2;	// 判斷範圍
	const double threshold = factor.pix_thresh;
	Seed_s seed = factor.seed;						// 初始seed

	double avg;
	unsigned long long cnt = 1;						// 計數成長的pixel數量

	Seed_s temp;								// 當前 判斷的周圍seed
	Seed_s current;								// 當前 判斷的中心seed
	std::queue<double> avg_que;					// 暫存某點成長判斷完，當下已成長區域的整體avg
	std::queue<Seed_s> sed_que;					// 暫存成長判斷為種子點的像素位置

	avg = m_pDoc->m_img[seed.z][(seed.y) * col + (seed.x)];
	src[seed.z][(seed.y) * col + (seed.x)] = obj;
	sed_que.push(seed);
	avg_que.push(avg);

	while (!sed_que.empty())
	{
		avg = avg_que.front();
		current = sed_que.front();

		// 判斷周圍區域是否符合成長標準
		// 並同時計算「已成長區域」的總平均
		register int i, j, k;
		for (k = -range; k <= range; ++k)
		{
			for (j = -range; j <= range; ++j)
			{
				for (i = -range; i <= range; ++i)
				{
					if ((current.x + i) < (col)			&& (current.x + i) >= 0 &&
						(current.y + j) < (row)			&& (current.y + j) >= 0 &&
						(current.z + k) < (totalSlice)	&& (current.z + k) >= 0)
					{
						if (src[current.z + k][(current.y + j) * col + (current.x + i)] == 0)
						{
							short n_pixel = 
								m_pDoc->m_img[current.z + k][(current.y + j) * col + (current.x + i)];

							if (abs(n_pixel - avg) <= threshold)
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								sed_que.push(temp);

								src[current.z + k][(current.y + j) * col + (current.x + i)] = obj;
								
								avg = (avg * cnt + n_pixel) / (cnt + 1);
								avg_que.push(avg);
								cnt += 1;
							}
							else
								src[current.z + k][(current.y + j) * col + (current.x + i)] = -obj;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

}

void C3DProcess::RG_3D_ConfidenceConnected(short** src, RG_factor& factor)
{
	// DO : 3D 區域成長
	// 利用當前區域的「平均值」與「標準差」界定成長標準，並以「像素強度」來判斷
	//
	int obj;
	if (m_spine) obj = 1;
	else if (m_sternum) obj = 3;
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const double sd_coeffi = factor.sd_coeffi;
	const double sd_thresh = factor.sd_thresh;
	const double pix_thresh = factor.pix_thresh;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	Seed_s	n_site;
	Seed_s	s_current;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	double	s_avg;
	unsigned long long	s_cnt = 0;
	unsigned long long  n_pixel = 0, s_pixel = 0;

	s_avg = imgPro[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = obj;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;

	auto outOfImg = [=](int px, int py, int pz)
	{	// 判斷有無超出影像邊界
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else
			return true;
	};
	
	while (!sed_que.empty())
	{
		register int si, sj, sk;
		s_avg = avg_que.front();
		s_current = sed_que.front();

		// 計算 總合 與 平均
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						sum +=
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						cnt += 1;
					}
				}
			}
		}
		n_avg = sum / cnt;

		// 計算 標準差
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						n_pixel =
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);
		
		// 制定、修正成長標準的上下限
		double up_limit = n_avg + (sd_coeffi * n_sd);
		double down_limit = n_avg - (sd_coeffi * n_sd);
		
		// 判斷是否符合成長標準
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
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

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = obj;
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
							else
								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = -obj;
						}
					}
				}
			}
		}
		sed_que.pop();
		avg_que.pop();
	}

}

void C3DProcess::RG_3D_ProposedMethod(short** src, RG_factor& factor)
{
	// DO : 3D 區域成長
	// 利用當前區域的「標準差」與「全域平均值」界定成長標準，並以「像素強度」來判斷
	//
	int obj;
	if (m_spine) obj = 1;
	else if (m_sternum) obj = 3;
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const double sd_coeffi = factor.sd_coeffi;
	const double sd_thresh = factor.sd_thresh;
	const double pix_thresh = factor.pix_thresh;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	Seed_s	n_site;
	Seed_s	s_current;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	double	s_avg;
	unsigned long long	s_cnt = 0;
	unsigned long long  n_pixel = 0, s_pixel = 0;

	s_avg = imgPro[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = obj;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;

	auto outOfImg = [=](int px, int py, int pz)
	{	// 判斷有無超出影像邊界
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else
			return true;
	};

	while (!sed_que.empty())
	{
		register int si, sj, sk;
		s_avg = avg_que.front();
		s_current = sed_que.front();

		// 計算 總合 與 平均
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						sum +=
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						cnt += 1;
					}
				}
			}
		}
		n_avg = sum / cnt;

		// 計算 標準差
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						n_pixel =
							imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);

		// 制定、修正成長標準的上下限
		double up_limit = n_avg + (sd_coeffi * n_sd);
		double down_limit = n_avg - (sd_coeffi * n_sd);

		// 判斷是否符合成長標準
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_current.x + si, s_current.y + sj, s_current.z + sk))
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] == 0)
						{
							n_pixel =
								imgPro[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];

							if ((n_sd <= sd_thresh && abs(n_pixel - s_avg) <= pix_thresh))
							{
								n_site.x = s_current.x + si;
								n_site.y = s_current.y + sj;
								n_site.z = s_current.z + sk;
								sed_que.push(n_site);

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = obj;
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
							else
								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = -obj;
						}
					}
				}
			}
		}
		sed_que.pop();
		avg_que.pop();
	}

}

void C3DProcess::RG_3D_Spine_process(short** src, RG_factor& factor)
{
	// DO : 3D - 脊椎 1111111111111111111111111111111111 次區域成長
	// 利用「全域平均值」與「標準差」界定成長標準，並以「限制線」避免成長溢出
	//
	const int obj = 1;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const double pix_th = factor.pix_thresh;
	const double sd_th = factor.sd_thresh;
	const double sd_co = factor.sd_coeffi;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	double	s_avg = 0;
	unsigned int s_cnt = 0, n_pixel = 0;
	Seed_s	s_pt, n_pt;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	auto lineFunc1 = [=](int px, int py, int pz)	// left line
	{
		float value = spine_line[pz].at(0).first * px + spine_line[pz].at(0).second - py;
		if (value <= 0)	return true;				// 在left line(要倒著看)的左邊
		else return false;
	};
	auto lineFunc2 = [=](int px, int py, int pz)	// right line
	{
		float value = spine_line[pz].at(1).first * px + spine_line[pz].at(1).second - py;
		if (value <= 0)	return true;				// 在right line(要倒著看)的右邊
		else return false;
	};
	auto outOfImg = [=](int px, int py, int pz)		// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};
	auto outOfRange = [=](int px, int py, int pz)	// 判斷限制線的範圍
	{
		//int x_l = spine_vertex[pz].at(0).first - 50;
		//int x_r = spine_vertex[pz].at(0).first + 50;
		//int y_d = spine_vertex[pz].at(0).second + 80;
		//int y_t = spine_vertex[pz].at(0).second;

		/*int x_l = spine_vertex[pz].at(1).first - 5;
		int x_r = spine_vertex[pz].at(2).first + 5;
		int y_d = max(spine_vertex[pz].at(1).second, spine_vertex[pz].at(2).second) + 5;
		int y_t = spine_vertex[pz].at(0).second;

		if (px >= x_l && px <= x_r && py <= y_d && py >= y_t)
			return false;
		else
			return true;*/

		if (pz >= 150 && pz <= 170)
			return false;
		else
			return true;
	};
	
	s_avg = imgPro[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = obj;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;

	// 做區域成長
	while (!sed_que.empty())
	{
		register int si, sj, sk;
		s_avg = avg_que.front();
		s_pt = sed_que.front();

		// 計算 總合 與 平均
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
					{
						sum +=
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];
						cnt += 1;
					}
				}
			}
		}
		n_avg = sum / cnt;

		// 計算 標準差
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
					{
						n_pixel =
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);

		// 判斷三維空間範圍內的像素點
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					// 判斷有無超出影像範圍且還沒判定過
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
						src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] == 0)
					{
						n_pixel =
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];

						if ( (n_sd <= sd_th && abs(n_pixel - s_avg) <= pix_th) )
						{
							if (!outOfRange(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
							{
								if (lineFunc1(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
									lineFunc2(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
								{
									n_pt.x = s_pt.x + si;
									n_pt.y = s_pt.y + sj;
									n_pt.z = s_pt.z + sk;
									sed_que.push(n_pt);

									s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
									avg_que.push(s_avg);
									s_cnt += 1;

									src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj;
								}
								else
								{
									src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj;
								}
							}
							else
							{
								n_pt.x = s_pt.x + si;
								n_pt.y = s_pt.y + sj;
								n_pt.z = s_pt.z + sk;
								sed_que.push(n_pt);

								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;

								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj;
							}
						}
						else
						{
							src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

}

void C3DProcess::RG2_3D_Spine_process(short** src, RG_factor& factor)
{
	// DO : 3D - 脊椎 2222222222222222222222222222222222 次區域成長
	// 利用「全域平均值」界定成長標準，並以「限制線」避免成長溢出
	//
	const int obj1 = 1; 
	const int obj2 = 2;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	double	th = factor.pix_thresh;
	double	co = factor.sd_coeffi;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	double	s_avg = 0;
	//double	up_limit, down_limit;
	unsigned int s_cnt = 0, n_pixel = 0;
	Seed_s	s_pt, n_pt;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;
	
	auto lineFunc1 = [=](int px, int py, int pz)	// left line
	{
		float value = spine_line[pz].at(0).first * px + spine_line[pz].at(0).second - py;
		if (value <= 0)	return true;				// 在left line(要倒著看)的左邊
		else return false;
	};
	auto lineFunc2 = [=](int px, int py, int pz)	// right line
	{
		float value = spine_line[pz].at(1).first * px + spine_line[pz].at(1).second - py;
		if (value <= 0)	return true;				// 在right line(要倒著看)的右邊
		else return false;
	};
	auto outOfImg = [=](int px, int py, int pz)		// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};
	auto outOfRange = [=](int px, int py, int pz)	// 判斷限制線的範圍
	{
		//int x_l = spine_vertex[pz].at(0).first - 50;
		//int x_r = spine_vertex[pz].at(0).first + 50;
		//int y_d = spine_vertex[pz].at(0).second + 80;
		//int y_t = spine_vertex[pz].at(0).second;
		int x_l = spine_vertex[pz].at(1).first - 5;
		int x_r = spine_vertex[pz].at(2).first + 5;
		int y_d = max(spine_vertex[pz].at(1).second, spine_vertex[pz].at(2).second) + 5;
		int y_t = spine_vertex[pz].at(0).second;
		if (px >= x_l && px <= x_r && py <= y_d && py >= y_t)
			return false;
		else
			return true;
	};
	
	// 先把第一次區域成長的種子點都加進來
	// 原本判定不要的先歸零
	register int si, sj, sk;
	for (sj = 0; sj < totalSlice; ++sj)
	{
		for (si = 0; si < totalXY; ++si)
		{
			if (src[sj][si] == obj1)
			{
				n_pt.x = si % col;
				n_pt.y = si / col;
				n_pt.z = sj;

				n_pixel = img[sj][si];
				s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
				s_cnt += 1;

				avg_que.push(s_avg);
				sed_que.push(n_pt);
			}
			else if (src[sj][si] == -obj1)
				src[sj][si] = 0;
		}
	}

	//sed_que.push(seed);
	//avg_que.push(s_avg);

	// 做第二次區域成長
	while (!sed_que.empty())
	{
		s_avg = avg_que.front();
		s_pt = sed_que.front();

		// 判斷三維空間範圍內的像素點
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					// 判斷有無超出影像範圍且還沒判定過
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
						src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] == 0)
					{
						n_pixel =
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];

						if (abs(n_pixel - s_avg) <= th)
						{
							if (!outOfRange(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
							{
								if (lineFunc1(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
									lineFunc2(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
								{
									n_pt.x = s_pt.x + si;
									n_pt.y = s_pt.y + sj;
									n_pt.z = s_pt.z + sk;
									sed_que.push(n_pt);

									s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
									avg_que.push(s_avg);
									s_cnt += 1;

									src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj2;
								}
								else
								{
									src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj2;
								}
							}
							else
							{
								n_pt.x = s_pt.x + si;
								n_pt.y = s_pt.y + sj;
								n_pt.z = s_pt.z + sk;
								sed_que.push(n_pt);

								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;

								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj2;
							}
						}
						else
						{
							src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj2;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}
	
}

void C3DProcess::RG_3D_Sternum_process(short** src, RG_factor& factor)
{
	// DO : 3D - 胸骨 1111111111111111111111111111111111 次區域成長
	// 利用「全域平均值」與「標準差」界定成長標準，並以「限制線」避免成長溢出
	//
	const int obj = 3;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	const double pix_th = factor.pix_thresh;
	const double sd_th = factor.sd_thresh;
	const double sd_co = factor.sd_coeffi;
	Seed_s	seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	double	s_avg = 0;
	unsigned int s_cnt = 0, n_pixel = 0;
	Seed_s	s_pt, n_pt;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	auto lineFunc1 = [=](int px, int py, int pz)	// left line
	{
		float value = sternum_line[pz].at(0).first * px + sternum_line[pz].at(0).second - py;
		if (value >= 0) return true;				// 在left line(要倒著看)的右邊
		else return false;
	};
	auto lineFunc2 = [=](int px, int py, int pz)	// right line
	{
		float value = sternum_line[pz].at(1).first * px + sternum_line[pz].at(1).second - py;
		if (value >= 0)	return true;				// 在right line(要倒著看)的右邊
		else return false;
	};
	auto outOfImg = [=](int px, int py, int pz)		// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};

	s_avg = imgPro[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = obj;
	avg_que.push(s_avg);
	sed_que.push(seed);
	s_cnt += 1;

	// 做區域成長
	while (!sed_que.empty())
	{
		register int si, sj, sk;
		s_avg = avg_que.front();
		s_pt = sed_que.front();

		// 計算 總合 與 平均
		double sum = 0, cnt = 0;
		double n_avg = 0, n_sd = 0;
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
					{
						sum +=
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];
						cnt += 1;
					}
				}
			}
		}
		n_avg = sum / cnt;

		// 計算 標準差
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
					{
						n_pixel =
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];
						n_sd += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_sd = sqrt(n_sd / cnt);

		// 判斷三維空間範圍內的像素點
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					// 判斷有無超出影像範圍且還沒判定過
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
						src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] == 0)
					{
						n_pixel =
							imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];

						if ((n_sd <= sd_th && abs(n_pixel - s_avg) <= pix_th))
						{
							if (lineFunc1(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) ||
								lineFunc2(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
							{
								n_pt.x = s_pt.x + si;
								n_pt.y = s_pt.y + sj;
								n_pt.z = s_pt.z + sk;
								sed_que.push(n_pt);

								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;

								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj;
							}
							else
							{
								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj;
							}
						}
						else
						{
							src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

}

void C3DProcess::RG2_3D_Sternum_process(short** src, RG_factor& factor)
{
	// DO : 3D - 胸骨 2222222222222222222222222222222222 次區域成長
	// 利用「全域平均值」與「標準差」界定成長標準，並以「限制線」避免成長溢出
	//
	const int obj1 = 3;
	const int obj2 = 4;
	const int row = ROW;
	const int col = COL;
	const int totalXY = ROW * COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	double th = factor.pix_thresh;
	double co = factor.sd_coeffi;
	Seed_s seed = factor.seed;
	BYTE**& img = m_pDoc->m_img;
	BYTE**& imgPro = m_pDoc->m_imgPro;

	double s_avg = 0;
	unsigned int s_cnt = 0, n_pixel = 0;
	Seed_s s_pt, n_pt;
	std::queue<Seed_s> sed_que;
	std::queue<double> avg_que;

	auto lineFunc1 = [=](int px, int py, int pz)	// left line
	{
		float value = sternum_line[pz].at(0).first * px + sternum_line[pz].at(0).second - py;
		if (value >= 0) return true;				// 在left line(要倒著看)的右邊
		else return false;
	};
	auto lineFunc2 = [=](int px, int py, int pz)	// right line
	{
		float value = sternum_line[pz].at(1).first * px + sternum_line[pz].at(1).second - py;
		if (value >= 0)	return true;				// 在right line(要倒著看)的右邊
		else return false;
	};
	auto outOfImg = [=](int px, int py, int pz)		// 影像邊界
	{
		if (px < col && px >= 0 && py < row && py >= 0 && pz < totalSlice && pz >= 0)
			return false;
		else return true;
	};

	// 先把第一次區域成長的種子點都先加進來
	// 原本判定不要的先歸零
	register int si, sj, sk;
	for (sj = 0; sj < totalSlice; ++sj)
	{
		for (si = 0; si < totalXY; ++si)
		{
			if (src[sj][si] == obj1)
			{
				n_pt.x = si % col;
				n_pt.y = si / col;
				n_pt.z = sj;

				n_pixel = img[sj][si];
				s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
				s_cnt += 1;

				avg_que.push(s_avg);
				sed_que.push(n_pt);
			}
			else if (src[sj][si] == -obj1)
				src[sj][si] = 0;
		}
	}

	// 做第二次區域成長
	while (!sed_que.empty())
	{
		s_avg = avg_que.front();
		s_pt = sed_que.front();

		// 判斷三維空間範圍內的像素點
		for (sk = -s_range; sk <= s_range; ++sk)
		{
			for (sj = -s_range; sj <= s_range; ++sj)
			{
				for (si = -s_range; si <= s_range; ++si)
				{
					// 判斷有無超出影像範圍且還沒判定過
					if (!outOfImg(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) &&
						src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] == 0)
					{
						n_pixel = imgPro[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)];
						if (abs(n_pixel - s_avg) <= th)
						{
							if (lineFunc1(s_pt.x + si, s_pt.y + sj, s_pt.z + sk) ||
								lineFunc2(s_pt.x + si, s_pt.y + sj, s_pt.z + sk))
							{
								n_pt.x = s_pt.x + si;
								n_pt.y = s_pt.y + sj;
								n_pt.z = s_pt.z + sk;
								sed_que.push(n_pt);

								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;

								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = obj2;
							}
							else
							{
								src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj2;
							}
						}
						else
						{
							src[s_pt.z + sk][(s_pt.y + sj) * col + (s_pt.x + si)] = -obj2;
						}
					}
				}
			}
		}
		avg_que.pop();
		sed_que.pop();
	}

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
	delete[] temp;
}

void C3DProcess::Dilation_3D(short** src, short element)
{
	// DO : 3D Dilation (膨脹 - 形態學處理)
	// 目前 6 與 18 鄰域 待改善 !!!
	//
	int obj1 = INT_MIN, obj2 = INT_MIN;
	if (m_spine)
	{
		obj1 = 1;
		if (get_spine) obj2 = 2;
	}
	else if (m_sternum)
	{
		obj1 = 3;
		if (get_sternum) obj2 = 4;
	}
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int total_z = Total_Slice;
	register int i, j, k;
	register int ni, nj, nk;

	// src : 原始以及將要被更改的矩陣
	// temp : 暫存原始狀態的矩陣(不做更動)
	short** temp = New2Dmatrix(total_z, total_xy, short);

	// Deep copy (目前先以這樣的方式處理QQ)
	//
	BYTE**& pro = m_pDoc->m_imgPro;
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
		for (k = 1; k < total_z - 1; ++k)
		{
			for (j = 1; j < row - 1; ++j)
			{
				for (i = 1; i < col - 1; ++i)
				{
					if (temp[k][j * col + i] > 0)
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
		for (k = 1; k < total_z - 1; ++k)
		{
			for (j = 1; j < row - 1; ++j)
			{
				for (i = 1; i < col - 1; ++i)
				{
					if (temp[k][j * col + i] > 0)
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
		// 遍歷所有像素點
		for (k = 1; k < total_z - 1; ++k)
		{
			for (j = 1; j < row - 1; ++j)
			{
				for (i = 1; i < col - 1; ++i)
				{
					// 僅膨脹已成長的區域
					if (temp[k][j * col + i] == obj1 || 
						temp[k][j * col + i] == obj2)
					{
						// 該點3*3*3周圍像素點
						for (nk = -1; nk <= 1; ++nk)
						{
							for (nj = -1; nj <= 1; ++nj)
							{
								for (ni = -1; ni <= 1; ++ni)
								{
									if (src[k + nk][(j + nj) * col + (i + ni)] <= 0 &&
										pro[k + nk][(j + nj) * col + (i + ni)] > 150)
										src[k + nk][(j + nj) * col + (i + ni)] = obj2;
								}
							}
						}
					}
				}
			}
		}
	}
	delete[] temp;
}

double C3DProcess::Calculate_Volume(short** src)
{
	int obj1, obj2;
	if (m_spine)
	{
		obj1 = 1;
		obj2 = 2;
	}
	else if (m_sternum)
	{
		obj1 = 3;
		obj2 = 4;
	}
	register int i, j;
	const int totalXY = COL * ROW;
	const int totalSlice = Total_Slice;
	unsigned long long n = 0;							// 計數成長的pixel數量
	for (j = 0; j < totalSlice; ++j)
	{
		for (i = 0; i < totalXY; ++i)
		{
			if (src[j][i] == obj1 || src[j][i] == obj2)
				n += 1;
		}
	}
	double volume = 0.0;		// 單位 (cm3)
	volume = (n * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z) / 1000;	
	return volume;
}

/*
	// 高通 濾波 (laplace filter)
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	BYTE**& pro = m_pDoc->m_imgPro;

	//std::vector<int> sharp_coef = {1, 1, 1, 1, -8, 1, 1, 1, 1};
	std::vector<int> sharp_coef = { 0, 1, 0, 1, -4, 1, 0, 1, 0 };
	const int weight = (sharp_coef[4] > 0) ? 1 : -1;

	auto sharpKernel = [=](BYTE* img, int slice, int x, int y)
	{
		int sum = 0, n = 0;
		for (int j = y - 1; j <= y + 1; ++j)
		{
			for (int i = x - 1; i <= x + 1; ++i)
			{
				sum += (sharp_coef[n] * img[j * col + i]);
				n += 1;
			}
		}
		return sum;
	};
	auto sharpFilter = [&](int start)
	{
		int slice = start, pixel = 0;
		while (slice < totalSlice)
		{
			BYTE* tmp = new BYTE[row * col];
			std::memcpy(tmp, pro[slice], sizeof(BYTE) * row * col);
			for (int j = 1; j < row-1; ++j)
			{
				for (int i = 1; i < col-1; ++i)
				{
					pixel = weight * sharpKernel(tmp, slice, i, j);
					pixel = tmp[j * col + i] + pixel;
					if (pixel > 255)	pixel = 255;
					else if (pixel < 0) pixel = 0;
					pro[slice][j * col + i] = pixel;
				}
			}
			delete[] tmp;
			slice += 6;
		}
		if (start == 0)	TRACE("Even Slice high Filter : Success!\n");
		else TRACE("Odd Slice high Filter : Success!\n");
	};

	if (!get_regionGrow)
	{
		start = clock();
		thread th0(sharpFilter, 0);
		thread th1(sharpFilter, 1);
		thread th2(sharpFilter, 2);
		thread th3(sharpFilter, 3);
		thread th4(sharpFilter, 4);
		thread th5(sharpFilter, 5);
		th0.join();	th1.join(); th2.join();
		th3.join(); th4.join(); th5.join();
		end = clock();
		TRACE1("Sharp Time : %f (s) \n\n", (double)(end - start) / CLOCKS_PER_SEC);
	}
*/

