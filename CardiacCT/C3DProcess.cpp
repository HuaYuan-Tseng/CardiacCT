
// C3DProcess.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "C3DProcess.h"
#include "CProgress.h"
#include "CWait.h"
#include "afxdialogex.h"
#include <ctime>
#include <queue>
#include <thread>
#include <numeric>
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
	DDX_Check(pDX, IDC_CHECK_COMPLETE, m_complete);
	DDX_Check(pDX, IDC_CHECK_HU_THRESHOLD, m_thresholdHU);
	DDX_Check(pDX, IDC_CHECK_PIXEL_THRESHOLD, m_thresholdPixel);

	DDX_Check(pDX, IDC_CHECK_3D_SEED, m_3Dseed);

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
	ON_BN_CLICKED(IDC_BUTTON_GROWING_CLEAR, &C3DProcess::OnBnClickedButtonGrowingClear)
	ON_BN_CLICKED(IDC_BUTTON_2DSEED_CLEAR, &C3DProcess::OnBnClickedButton2dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_3DSEED_CLEAR, &C3DProcess::OnBnClickedButton3dseedClear)
	ON_BN_CLICKED(IDC_BUTTON_SEED_CHANGE, &C3DProcess::OnBnClickedButtonSeedChange)

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
	// Dialog ��l�ƪ��ƥ�
	//

	// ��Dialog���D
	//
	SetWindowText("3D Processing");

	// �]�wø�Ϯy�жb�B�ϰ�P����
	//
	m_2D_frame = GetDlgItem(IDC_STATIC_2D);									// �X�� Dialog ��������C
	m_2D_frame->GetWindowRect(&m_2D_rect);									// ��o����b�ù��W����m�]�H�ù����y�жb�^�C
	ScreenToClient(&m_2D_rect);												// �N����b�H"�ù�"���y�жb��t�U����m�A�ഫ��"�Τ��"�y�жb��t�U����m�C
	//m_2D_frame->MoveWindow(m_2D_rect.left, m_2D_rect.top, COL, ROW, true);// �վ㪫���m�P�j�p�C

	m_3D_frame = GetDlgItem(IDC_STATIC_3D);
	m_3D_frame->GetWindowRect(&m_3D_rect);
	ScreenToClient(&m_3D_rect);
	//m_3D_frame->MoveWindow(m_3D_rect.left, m_3D_rect.top, COL, ROW, true);

	m_2D_dib = new CDIB();							
	m_2D_dib->InitDIB(COL, ROW);											// ��l�Ƶe��

	// �]�wScrollBar���d��
	//
	m_ScrollBar.SetScrollRange(0, Total_Slice-1);

	//-------------------------------------------------------------------------------------//
	// �]�wButton��l���A
	//
	GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);

	//-------------------------------------------------------------------------------------//
	// ��l�Ư��z�x�}�H�ΰϰ즨���P�w���x�}�j�p�M��l��
	//
	register int i, j;
	int totalx = ROW * COL;
	int totaly = Total_Slice;
	judge = New2Dmatrix(Total_Slice, ROW * COL, BYTE);

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
	// ���T���v��Z�b��V���Y����(���n�h�áA�諸�OZ�b�S��)
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
	// �]�w�ե��Ѽ�
	//
	x_index = (2.0F / 511.0F) * (COL / 2.0F) / 2;
	x_index = x_index / scale_y;
	y_index = (2.0F / 511.0F) * (ROW / 2.0F) / 2;
	y_index = y_index / scale_z;
	z_index = (2.0F / 511.0F) * (Total_Slice / 2.0F) / 2;
	z_index = z_index / scale_x;

	//-------------------------------------------------------------------------------------//
	// openGL�Ŷ��إ�
	//
	m_hDC = ::GetDC(m_3D_frame->m_hWnd);					// ��o�e������DC��HANDLE�]hDC�^
	SetupPixelFormat(m_hDC);
	if ((m_hRC = ::wglCreateContext(m_hDC)) == 0)			// ���� openGL �һݪ��e���]hRC�^
	{
		AfxMessageBox("Fail to create hRC context!");
		return FALSE;
	}
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)			// �إ� hDC �P hRC �������s��
	{
		AfxMessageBox("Fail to make current!");
		return FALSE;
	}
	PerspectiveBuild();										// �إ�openGL�z���Ŷ�
	gl_3DTexture = ExtensionSupported("GL_EXT_texture3D");	// �T�{�O�_�䴩3D���z
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
	// ø�Ϩƥ�
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
	// �ƹ��u���ƥ�
	//
	POINT mpt;						// ��o���з�U��m
	GetCursorPos(&mpt);
	ScreenToClient(&mpt);
	int TotalSlice = Total_Slice;

	// �b �G�� �v�������d��
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

	// �b �T�� �v�������d��
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
	// ScrollBar �������ʨƥ�
	//
	int n = m_ScrollBar.GetScrollPos();			// n ���P�� slice
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
	// �ƹ����ʨƥ�
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
	// �ƹ����� ���U �ƥ�
	//
	// �T�� �v������
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

				seed_img = coordiConvert(seed_gl);		// �y���I�P��Ưx�}��m���ഫ

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
	// �G�� �v������
	//
	if (point.x < m_2D_rect.right && point.x > m_2D_rect.left && point.y < m_2D_rect.bottom && point.y > m_2D_rect.top)
	{
		seed_pt.x = (short)(point.x - m_2D_rect.left);
		seed_pt.y = (short)(point.y - m_2D_rect.top);
		seed_pt.z = DisplaySlice;

		m_pos_1.Format("%d", (int)seed_pt.x);
		m_pos_2.Format("%d", (int)seed_pt.y);
		m_pos_3.Format("%d", (int)seed_pt.z);
		m_pos_4.Format("%d", (int)m_pDoc->m_img[seed_pt.z][(seed_pt.y * 512) + seed_pt.x]);

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
	// �ƹ����� ��} �ƥ�
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
	// �ƹ��k�� ���U �ƥ�
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
	// �ƹ��k�� ��} �ƥ�
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
	// �ƹ��k�� ����U �ƥ�
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
	// �� �G�Ȥ��H�� (Pixel)
	// EditBox : Pixel Threshold (m_pixelThreshold)
	//
	UpdateData(TRUE);
	PixelThreshold = atoi(m_pixelThreshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditHuThreshold()
{
	// �� �G�Ȥ��H�� (HU)
	// EditBox : HU Threshold (m_HUThreshold)
	//
	UpdateData(TRUE);
	HUThreshold = atoi(m_HUThreshold);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditSlices()
{
	// ��openGLø�s�T���v�������z�h��
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
			GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
		}
	}
	else
	{
		GetDlgItem(IDC_BUTTON_REGION_GROWING)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_3DSEED_CLEAR)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SEED_CHANGE)->EnableWindow(FALSE);
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
	// (������...)
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
		m_pos_8.Format("%d", (int)(m_pDoc->m_img[seed_pt.z][(seed_pt.y * 512) + seed_pt.x]));

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
	if (get_3Dseed)
	{
		clock_t start, end;
		CWait* m_wait = new CWait();
		m_wait->Create(IDD_DIALOG_WAIT);
		m_wait->ShowWindow(SW_NORMAL);
		m_wait->setDisplay("Region growing...");

		// �ŧi ��������
		//
		RG_totalTerm = {
			RG_totalTerm.seed = seed_img,
			RG_totalTerm.s_kernel = 5,
			RG_totalTerm.n_kernel = 3,
			RG_totalTerm.threshold = 20.0L,
			RG_totalTerm.coefficient = 2.5L
		};
		
		// ���� 3D_Region growing
		//
		start = clock();
		//RG_3D_GlobalAvgConnected(judge, RG_totalTerm);
		//RG_3D_LocalAvgConnected(judge, RG_totalTerm);
		RG_3D_ConfidenceConnected(judge, RG_totalTerm);
		end = clock();
		//RG_totalVolume = Calculate_Volume(judge, 1);
		//TRACE1("Org Growing Volume : %f (cm3) \n", RG_totalVolume);
		TRACE1("RG Time : %f (s) \n", (double)((end - start)) / CLOCKS_PER_SEC);

		// 3D_�κA�ǳB�z
		//
#if 0
		start = clock();
		//Erosion_3D(judge, 18);
		//Dilation_3D(judge, 18);
		//RG_3D_Link(judge, RG_totalTerm);
		//Dilation_3D(judge, 26);
		//Dilation_3D(judge, 26);
		end = clock();
		TRACE1("Morphology Time : %f (s) \n", (double)((end - start)) / CLOCKS_PER_SEC);
#endif

		// �T�{�̫������n
		//
		get_regionGrow = true;
		RG_totalVolume = Calculate_Volume(judge, 1);
		m_result.Format("%lf", RG_totalVolume);
		TRACE1("Last Growing Volume : %f (cm3) \n", RG_totalVolume);
		
		PrepareVolume();
		UpdateData(FALSE);
		Draw3DImage(true);
		Draw2DImage(DisplaySlice);
		GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(TRUE);
		m_wait->DestroyWindow();
		delete m_wait;
	}
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
	GetDlgItem(IDC_BUTTON_GROWING_CLEAR)->EnableWindow(FALSE);
	//------------------------------------------------------------//

	float pixel;
	const int row = ROW;
	const int col = COL;
	const int totalx = ROW * COL;
	const int totaly = Total_Slice;
	const int totalSlice = Total_Slice;
	const int sample_start = 0 + Mat_Offset;
	const int sample_end = 0 + Mat_Offset + totalSlice;

	// ��_ : �����P�w�x�}
	//
	register int i, j, k;
	for (j = 0; j < totaly; j++)
	{
		for (i = 0; i < totalx; i++)
		{
			judge[j][i] = 0;
		}
	}

	// ��_ : �v��
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

//==========================//
//   C3DProcess Functions   //
//==========================//

BOOL C3DProcess::SetupPixelFormat(HDC hDC)
{
	// DO : �]�m(��l��)hDC���������c
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

	// �]�m�����榡
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

	// ���չ����榡
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
	// DO : �X�i�䴩�T�{
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
	// DO : �إ�openGL�z���Ŷ�
	//
	GLint	gl_x, gl_y;
	gl_x = m_3D_rect.right - m_3D_rect.left;
	gl_y = m_3D_rect.bottom - m_3D_rect.top;

	glViewport(0, 0, gl_x, gl_y);									// �]�w�ù��������f��ܽd��
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();												// �����e�x�}�ର���x�}
	gluPerspective(40.0, (GLfloat)gl_x/(GLfloat)gl_y, 0.5, 10.0);	// �إ߹�٪��z����ı�Ŷ�
	glMatrixMode(GL_MODELVIEW);
}

void C3DProcess::GLInitialization()
{
	// DO : openGLø�Ϫ�l��
	//
	if (::wglMakeCurrent(m_hDC, m_hRC) == FALSE)
	{
		AfxMessageBox("Fail to make current!");
		return;
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	 
	// �إ�3D���z�x�s�Ŷ�
	//
	glGenTextures(ImageFrame, textureName);					// �i�D openGL �t�m�@���O����Ŷ��s�����
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);					// ���� Ū��(or�ǿ�) �����ƾڹ���覡 ��
															// �w�] 4 bytes�A�]�� 1 byte �קK padding
	// ���J���z
	//
	PrepareVolume();

	// �إ߳��I�y��(�إ�openGLø�ϮɡAglVertex()�һݭn���I)
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
	// DO : �إ߯��z����Ưx�}
	//
	if (m_pDoc->m_img == NULL)	return;

	// �w�ƭn�Ψӫإ߯��z����Ưx�}
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
	m_progress->Set(Total_Slice/2, 0);
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
			m_progress->GetPro(k);
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

						if (judge[k - (Mat_Offset + 1)][j * col + i] == 0)
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
			m_progress->GetPro(k);
		}
	}
	
	LoadVolume();
	m_progress->DestroyWindow();
	delete m_progress;
}

void C3DProcess::LoadVolume()
{
	// DO : �إ�3D���z
	//
	if (gl_3DTexture)
	{
		float color[] = { 1.0F, 1.0F, 1.0F, 1.0F };

		// ���z�y�Шt�ΡGS����b�BT���a�b�BR�����J�ù����b
		// �]�m���z��¶�Ҧ�
		//
		glBindTexture(GL_TEXTURE_3D, textureName[0]);							// �j�w���z�]���w����, ���z��HID�^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// ���z�L�o��ơ]����o��^
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// ��j�ɪ��o��覡
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		// �Y�p�ɪ��o��覡
		glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, (GLvoid*)m_image0);								// �Ы�3D���z
	}
}

void C3DProcess::getRamp(GLubyte* color, float t, int n)
{
	// DO : �p��RGBA���ƭ�
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
	// DO : ø�s 3D �v��
	//
	CClientDC dc(m_3D_frame);

	// openGL �q�{ 4X4 �x�}�P�O���� 1D �}�C���Y
	// �z				 �{
	// ��	 m0	 m4  m8  m12 ��	
	// �� m1	 m5  m9  m13 ��
	// ��	 m2  m6  m10 m14 ��
	// �� m3  m7  m11 m15 ��
	// �|				 �}
	// (m12, m13, m14)�O�Χ@ Translation�A(m15)�O�����y�С]�Χ@ Projection�^�A
	// ���W 9 �Ӥ����Χ@ Rotate �M Scale�C

	// Xform matrices(����᪺�ҫ��x�})
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

	// clip planes equation�]A, B, C, Z�^
	// Ax + By + Cz + D = 0�A�p�G�O(0, -1, 0, 0)�A
	// �N��O y<0 ���~����ܡC
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
	glEnable(GL_DEPTH_TEST);								// �Ұʲ`�״���(�S���}�Ҫ��ܡA��Ӫ���|���I�z��)

	// �إ��[�ݪ��� �z��������B������V�B�Z��(viewDistance)
	//
	gluPerspective(90, 1, 1, 700);							// ����(���ѱ�)�o�ӡA3D seed ���ĪG�N�|���n��~
	glMatrixMode(GL_MODELVIEW);								// �ҫ����ϯx�}
	glLoadIdentity();										// testmat[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1]
	glTranslatef(0.0f, 0.0f, viewDistance);					// testmat[1,0,0,0, 0,1,0,0, 0,0,1,-4, 0,0,0,1]

	// ���� ��Ŧ ����
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
	glMultMatrixf((GLfloat *)objectXform);					// ������(��Ŧ)����A�R����N�S����k����F

	// ���� ���ӥ��� ����
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

	// �u�ݭn Rotation�A�ҥH�N Translation �]�� 0
	//
	glGetFloatv(GL_MODELVIEW_MATRIX, mat);
	InvertMat(mat);

	mat[12] = 0.0F;
	mat[13] = 0.0F;
	mat[14] = 0.0F;

	// ��o user_Plane ��������{���Y��
	// �� ���ӥ��� �b�ۨ����Шt�� x �b��V�첾�ɡA
	// �� ����(��Ŧ) �H ���ӥ��� �� x �b ���ʤ�V�ѭ�]�������^�C
	//
	user_Plane[0] = -planeXform[0];
	user_Plane[1] = -planeXform[1];
	user_Plane[2] = -planeXform[2];

	// �۰ʥͦ����z�y�Шt
	//
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

	// �إߵ��ť���(Clip plane)���|�Ө������I
	// �p�� clip plane �|�Ө������I plane[16]={1,-1,-1, 1,-1,1, 1,1,-1, 1,1,1}
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

	// �M��P�[���̬ۤϪ����ť���
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

	// �t�m���ť��� 0 ~ 5
	//
	glClipPlane(GL_CLIP_PLANE0, clip0);
	glClipPlane(GL_CLIP_PLANE1, clip1);
	glClipPlane(GL_CLIP_PLANE2, clip2);
	glClipPlane(GL_CLIP_PLANE3, clip3);
	glClipPlane(GL_CLIP_PLANE4, clip4);
	glClipPlane(GL_CLIP_PLANE5, clip5);

	// �N�P�[���̬ۤϪ����ť����]�w���[���̾ާ@�� ���ӥ���
	//
	glClipPlane(GL_CLIP_PLANE0 + clip, user_Plane);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);
	glEnable(GL_CLIP_PLANE4);
	glEnable(GL_CLIP_PLANE5);

	glColor4f(1.0F, 1.0F, 1.0F, intensity);				// �]�wopenGL��slcies�C��]R, G, B, Alpha�^

	// �Ұ� Alpha / �V�X ����
	//
	glEnable(GL_ALPHA_TEST);							// �ҥ� Alpha ����
	glAlphaFunc(GL_GREATER, density*intensity);			// �]�w Alpha ���ժ��Ѧҭ�

	glEnable(GL_BLEND);									// �ҥ� �C��V�X
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// �]�w �C��V�X(Source and Target)

	// �]�w���z�y�Шt��
	//
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glEnable(GL_TEXTURE_3D);

	glTranslatef(0.5F, 0.5F, 0.5F);						// �N����(��Ŧ)���쥿����
	glScalef(scale_x, scale_y, scale_z);				// �N����(��Ŧ)�Y��

	glMultMatrixf(mat);
	glTranslatef(0.0F, 0.0F, -viewDistance);

	// set modelView to identity
	//
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	{
		glLoadIdentity();

		// �]�w���z�y�Шt�Ϊ��ͦ�
		//
		glTexGenfv(GL_S, GL_EYE_PLANE, xPlane);
		glTexGenfv(GL_T, GL_EYE_PLANE, yPlane);
		glTexGenfv(GL_R, GL_EYE_PLANE, zPlane);

		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);

		glBindTexture(GL_TEXTURE_3D, textureName[0]);

		glTranslatef(0.0f, 0.0f, viewDistance);

		// ø�sopenGL���C�@�hslices
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

	// ø�s�P���Ŧ����
	//
	glLineWidth(3);
	glBegin(GL_LINES);
	{
		glColor3f(0.0f, 0.0f, 1.0f);

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

	// ø�s�[���̪�����
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

	// ��� 3D seed
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
	//	DO : ø�s 2D �v��
	//
	CClientDC dc(m_2D_frame);

	const int Row = ROW;
	const int Col = COL;
	register int i, j;

	if (m_pDoc->m_img == nullptr)
	{
		AfxMessageBox("No Image Data can be display!!");
		return;
	}

	// ��ܭ�l�v��
	//
	if (m_complete == TRUE)
	{
		m_2D_dib->ShowInverseDIB(&dc, m_pDoc->m_img[slice]);
	}
	// ��ܥHPixel���H�Ȫ��G�ȤƼv��
	//
	else if (m_thresholdPixel == TRUE)
	{
		PBYTE image_thres = new BYTE[Row*Col];

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
	// ��ܥH HU ���H�Ȫ��G�ȤƼv��
	//
	else if (m_thresholdHU == TRUE)
	{
		PBYTE image_thres = new BYTE[Row*Col];

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

	// ��2D�v����ܦb2D�I�諸�ؤl�I
	//
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

	//  3D seed �\��
	//
	if (m_3Dseed)
	{
		CPoint pt;
			
		// ��2D�v����ܦb3D�ϰ즨�����G
		//
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
				}
			}
		}
		// ��2D�v����ܦb3D�I�諸seed
		//
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
	
	// �g�r (slice)
	//
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
	// DO : ���U�ƹ���}�l���ʧ@
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
	// DO : �񱼷ƹ��᪺�ʧ@
	// �t�~�Τ@��bool���ܼƨӧP�_�ƹ�����O�]���񱼷ƹ��ɡAnFlags���O0�C
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
	// DO : ����ƹ��ɩҰ��檺�ʧ@
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
	// ���]�ߤ����.�e.�����O 512 (�H�x�}�ӻ�)�A���I��m���O�q -1 �� 1 (�HGL�Ŷ��ӻ�)�A
	// �h����ؤl�I���Y�b��m�ɡA���p��x�}�PGL�Ŷ������ �A�A�p��x�}(�Τ���)�@�b���ƶq�A
	// �̫��̬ۭ�����ᰣ�H 2 �A�N�O��l�v����ݬY�@�ݦb openGL �Ŷ����b��m�A
	// ���]���v���٦����Y��A�ҥH���H�Y���ҫ���o�����G�A�~�O���ݯu�����b��m�A
	// ���ۡA�]����ݦ�m�O������Y�A�ҥH�̫�Q�άY�@�ݺݦ�m�B�t�@�ݦ�m�B�x�}�ơB
	// �H�β{�b�ؤl�I���b��m�h���T����Ҵ���A��o�����G�Y���ؤl�I��������m�C
	
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
	// �ʺA�t�m�G���x�}
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

void C3DProcess::RG_3D_GlobalAvgConnected(BYTE** src, RG_factor& factor)
{
	//	DO : 3D �ϰ즨�� 
	//	�Q�Ρu��e�w���������쥭���ȡv�Ӭɩw�����зǡA�åΡu�ثe�������j�סv�ӧP�_
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int range = (factor.s_kernel - 1) / 2;	// �P�_�d��
	register int i, j, k;
	unsigned int cnt = 1;							// �p�Ʀ�����pixel�ƶq
	short n_pixel = 0;
	
	double avg;
	double up_limit;
	double down_limit;
	double threshold = factor.threshold;
	
	Seed_s temp;								// ��e �P�_���P��seed
	Seed_s current;								// ��e �P�_������seed
	Seed_s seed = factor.seed;					// ��lseed
	queue<double> avg_que;						// �Ȧs�Y�I�����P�_���A��U�w�����ϰ쪺����avg
	queue<Seed_s> sd_que;						// �Ȧs�����P�_���ؤl�I��������m

	avg = m_pDoc->m_img[seed.z][(seed.y) * col + (seed.x)];
	src[seed.z][(seed.y) * col + (seed.x)] = 1;
	avg_que.push(avg);
	sd_que.push(seed);

	while (!sd_que.empty())
	{
		avg = avg_que.front();
		current = sd_que.front();
		up_limit = avg + threshold;
		down_limit = avg - threshold;

		// ���s�ɩw�����зǤW�U��
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

		// �P�_�P��ϰ�O�_�ŦX�����з�
		// �æP�ɭp��u�w�����ϰ�v���`����
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
						if (src[current.z + k][(current.y + j) * col + (current.x + i)] != 1)
						{
							n_pixel = m_pDoc->m_img[current.z + k][(current.y + j) * col + (current.x + i)];

							if ((n_pixel <= up_limit) && (n_pixel >= down_limit))
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								sd_que.push(temp);

								src[current.z + k][(current.y + j) * col + (current.x + i)] = 1;
								
								avg = (avg * cnt + n_pixel) / (cnt + 1);
								avg_que.push(avg);
								cnt += 1;
							}
						}
					}
				}
			}
		}
		avg_que.pop();
		sd_que.pop();
	}
	//TRACE1("sd : %d \n", sd_que.size());
}

void C3DProcess::RG_3D_LocalAvgConnected(BYTE** src, RG_factor& factor)
{
	//	DO : 3D �ϰ즨��
	//	�Q�Ρu��e�w���������쥭���ȡv�Ӭɩw�����зǡA�åΡu�ثe�����F��ϰ쪺�����ȡv�ӧP�_
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
	queue<Seed_s>	sd_que;
	queue<double>	avg_que;
	
	s_avg = m_pDoc->m_img[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = 1;
	avg_que.push(s_avg);
	sd_que.push(seed);

	while (!sd_que.empty())
	{
		s_avg = avg_que.front();
		s_current = sd_que.front();
		up_limit = s_avg + threshold;
		down_limit = s_avg - threshold;

		// ���s�ɩw�����з�
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

		// �P�_�P��ϰ�
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
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] != 1)
						{
							n_current.x = s_current.x + si;
							n_current.y = s_current.y + sj;
							n_current.z = s_current.z + sk;
							n_pixel = 0;	n_cnt = 0;

							// �p��P��ϰ칳���j�ת�������
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

							// �P�_�O�_�ŦX�����з�
							if ((n_avg <= up_limit) && (n_avg >= down_limit))
							{
								sd_que.push(n_current);
								src[n_current.z][n_current.y * col + n_current.x] = 1;
								n_pixel = m_pDoc->m_img[n_current.z][n_current.y * col + n_current.x];
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
						}
					}
				}
			}
		}
		sd_que.pop();
		avg_que.pop();
	}

}

void C3DProcess::RG_3D_ConfidenceConnected(BYTE** src, RG_factor& factor)
{
	// DO : 3D �ϰ즨��
	// �Q�η�e�ϰ쪺�u�����ȡv�P�u�зǮt�v�ɩw�����зǡA�åH�u�����j�סv�ӧP�_
	//
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	const int s_range = (factor.s_kernel - 1) / 2;
	register int si, sj, sk;
	unsigned int n_cnt = 0;
	unsigned int s_cnt = 0;
	int n_pixel = 0;

	double	n_SD;
	double	n_avg;
	double	s_avg; 
	double	up_limit;
	double	down_limit;
	double  n_pixel_sum = 0;
	double	threshold = factor.threshold;
	double	coefficient = factor.coefficient;

	Seed_s	n_site;
	Seed_s	s_current;
	Seed_s	seed = factor.seed;
	queue<Seed_s> sd_que;
	queue<double> avg_que;

	s_avg = m_pDoc->m_img[seed.z][seed.y * col + seed.x];
	src[seed.z][seed.y * col + seed.x] = 1;
	avg_que.push(s_avg);
	sd_que.push(seed);
	s_cnt += 1;

	// �G��
	//
	//while (!sd_que.empty())
	//{
	//	s_avg = avg_que.front();//
	//	s_current = sd_que.front();
	//	n_pixel_sum = 0, n_cnt = 0, n_avg = 0, n_SD = 0;
	//
	//	// �p�� �`�X �P ����
	//	for (sj = -s_range; sj <= s_range; sj++)
	//	{
	//		for (si = -s_range; si <= s_range; si++)
	//		{
	//			if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
	//				(s_current.y + sj) < row && (s_current.y + sj) >= 0 )
	//			{
	//				n_pixel_sum +=
	//					m_pDoc->m_img[s_current.z][(s_current.y + sj) * col + (s_current.x + si)];
	//				n_cnt += 1;
	//			}
	//		}
	//	}
	//	n_avg = (double)n_pixel_sum / n_cnt;
	//
	//	// �p�� �зǮt
	//	for (sj = -s_range; sj <= s_range; sj++)
	//	{
	//		for (si = -s_range; si <= s_range; si++)
	//		{
	//			if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
	//				(s_current.y + sj) < row && (s_current.y + sj) >= 0 )
	//			{
	//				n_pixel =
	//					m_pDoc->m_img[s_current.z][(s_current.y + sj) * col + (s_current.x + si)];
	//				n_SD += pow((n_pixel - n_avg), 2);
	//			}
	//		}
	//	}
	//	n_SD = sqrt(n_SD / n_cnt);
	//
	//	// �P�_�O�_�ŦX�����з�
	//	up_limit = n_avg + n_SD;
	//	down_limit = n_avg - n_SD;
	//	for (sj = -s_range; sj <= s_range; sj++)
	//	{
	//		for (si = -s_range; si <= s_range; si++)
	//		{
	//			if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
	//				(s_current.y + sj) < row && (s_current.y + sj) >= 0 )
	//			{
	//				if (src[s_current.z][(s_current.y + sj) * col + (s_current.x + si)] != 1)
	//				{
	//					n_pixel =
	//						m_pDoc->m_img[s_current.z][(s_current.y + sj) * col + (s_current.x + si)];
	//					if (n_pixel <= up_limit && n_pixel >= down_limit && abs(n_pixel - s_avg) <= 50 )
	//					{
	//						n_site.x = s_current.x + si;
	//						n_site.y = s_current.y + sj;
	//						n_site.z = s_current.z;
	//						sd_que.push(n_site);
	//
	//						src[s_current.z][(s_current.y + sj) * col + (s_current.x + si)] = 1;
	//						
	//						s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);//
	//						avg_que.push(s_avg);//
	//						s_cnt += 1;//
	//					}
	//				}
	//			}
	//		}
	//	}
	//	sd_que.pop();
	//	avg_que.pop();//
	//}
	
	// �T��
	//
	while (!sd_que.empty())
	{
		s_avg = avg_que.front();
		s_current = sd_que.front();
		n_pixel_sum = 0, n_cnt = 0, n_avg = 0, n_SD = 0;
		
		// �p�� �`�X �P ����
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
						(s_current.y + sj) < row && (s_current.y + sj) >= 0 &&
						(s_current.z + sk) < totalSlice && (s_current.z + sk) >= 0)
					{
						n_pixel_sum += 
							m_pDoc->m_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_cnt += 1;
					}
				}
			}
		}
		n_avg = n_pixel_sum / n_cnt;

		// �p�� �зǮt
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
						(s_current.y + sj) < row && (s_current.y + sj) >= 0 &&
						(s_current.z + sk) < totalSlice && (s_current.z + sk) >= 0)
					{
						n_pixel =
							m_pDoc->m_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
						n_SD += pow((n_pixel - n_avg), 2);
					}
				}
			}
		}
		n_SD = sqrt(n_SD / n_cnt);

		// �P�_�O�_�ŦX�����з�
		up_limit = n_avg + coefficient * n_SD;
		down_limit = n_avg - coefficient * n_SD;
		for (sk = -s_range; sk <= s_range; sk++)
		{
			for (sj = -s_range; sj <= s_range; sj++)
			{
				for (si = -s_range; si <= s_range; si++)
				{
					if ((s_current.x + si) < col && (s_current.x + si) >= 0 &&
						(s_current.y + sj) < row && (s_current.y + sj) >= 0 &&
						(s_current.z + sk) < totalSlice && (s_current.z + sk) >= 0)
					{
						if (src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] != 1)
						{
							n_pixel =
								m_pDoc->m_img[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)];
							
							if (n_pixel <= up_limit && n_pixel >= down_limit && abs(n_pixel - s_avg) <= threshold)
							{ 
								n_site.x = s_current.x + si;
								n_site.y = s_current.y + sj;
								n_site.z = s_current.z + sk;
								sd_que.push(n_site);

								src[s_current.z + sk][(s_current.y + sj) * col + (s_current.x + si)] = 1;
								s_avg = (s_avg * s_cnt + n_pixel) / (s_cnt + 1);
								avg_que.push(s_avg);
								s_cnt += 1;
							}
						}
					}
				}
			}
		}
		avg_que.pop();
		sd_que.pop();  
	}
	

}

void C3DProcess::RG_3D_Link(BYTE** src, RG_factor& factor)
{
	//	DO : 3D �ϰ즨��
	//	�N�P�ؤl�I�s���������I�u�s���v�_��(�T�{�̲פ��ΰϰ�P��n)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int totalSlice = Total_Slice;
	const int kernel = 3;
	const int range = (kernel - 1) / 2;			// �P�_�d��
	register int i, j, k;

	// src : ��l�H�αN�n�Q��諸�x�}
	// temp : �Ȧs��l���A���x�}(�������)
	BYTE** src_temp = New2Dmatrix(totalSlice, total_xy, BYTE);

	Seed_s temp;								// ��e �P�_���P��seed
	Seed_s current;								// ��e �P�_������seed
	Seed_s seed = factor.seed;					// ��lseed
	queue<Seed_s> sd_que;						// �Ȧs�����P�_���ؤl�I��������m

	// Deep copy (�ثe���H�o�˪��覡�B�zQQ)
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

void C3DProcess::Erosion_3D(BYTE** src, short element)
{
	// DO : 3D Erosion (�I�k - �κA�ǳB�z)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int total_z = Total_Slice;
	register int i, j, k;

	// src : ��l�H�αN�n�Q��諸�x�}
	// temp : �Ȧs��l���A���x�}(�������)
	BYTE** temp = New2Dmatrix(total_z, total_xy, BYTE);
	
	// Deep copy (�ثe���H�o�˪��覡�B�zQQ)
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
	// 6 �F��
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
	// 18 �F��
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
	// 26 �F��
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

void C3DProcess::Dilation_3D(BYTE** src, short element)
{
	// DO : 3D Dilation (���� - �κA�ǳB�z)
	//
	const int row = ROW;
	const int col = COL;
	const int total_xy = ROW * COL;
	const int total_z = Total_Slice;
	register int i, j, k;

	// src : ��l�H�αN�n�Q��諸�x�}
	// temp : �Ȧs��l���A���x�}(�������)
	BYTE** temp = New2Dmatrix(total_z, total_xy, BYTE);

	// Deep copy (�ثe���H�o�˪��覡�B�zQQ)
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
	// 6 �F��
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
	// 18 �F��
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
	// 26 �F��
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

double C3DProcess::Calculate_Volume(BYTE** src, short target)
{
	const int row = ROW;
	const int col = COL;
	const int totalSlice = Total_Slice;
	register int i, j, k;
	unsigned int n = 0;							// �p�Ʀ�����pixel�ƶq
	double volume = 0L;

	for (k = 0; k < totalSlice; k++)
	{
		for (j = 0; j < row; j++)
		{
			for (i = 0; i < col; i++)
			{
				if (src[k][j * col + i] == target)
					n += 1;
			}
		}
	}
	volume = (n * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z) / 1000;		// ��� (cm3)
	return volume;
}