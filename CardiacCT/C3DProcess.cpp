
// C3DProcess.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "C3DProcess.h"
#include "CProgress.h"
#include "afxdialogex.h"

#define M_PI 3.1415926f
#define New2Dmatrix(H, W, TYPE)	(TYPE**)new2Dmatrix(H, W, sizeof(TYPE))
#define New3Dmatrix(H, W, L, TYPE) (TYPE***)new3Dmatrix(H, W, L, sizeof(TYPE))
#define New4Dmatrix(H, W, L, V, TYPE) (TYPE****)new4Dmatrix(H, W, L, V, sizeof(TYPE))

#define Display_Series m_pDoc->displaySeries
#define Total_Slice m_pDoc->m_dir->SeriesList[0]->TotalSliceCount
#define ROW m_pDoc->m_dir->SeriesList[0]->ImageList[0]->Row
#define COL m_pDoc->m_dir->SeriesList[0]->ImageList[0]->Col
#define Window_Center_1 m_pDoc->m_dir->Window_1_Center
#define Window_Center_2 m_pDoc->m_dir->Window_2_Center
#define Window_Width_1 m_pDoc->m_dir->Window_1_Width
#define Window_Width_2 m_pDoc->m_dir->Window_2_Width
#define Rescale_Intercept atoi(m_pDoc->m_dir->Rescale_Intercept)
#define Rescale_Slope atoi(m_pDoc->m_dir->Rescale_Slope)

//==========================//
//   3D Processing Dialog   //
//==========================//

IMPLEMENT_DYNAMIC(C3DProcess, CDialogEx)

C3DProcess::C3DProcess(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_3DPROCESS, pParent)
	, m_object(TRUE)
	, m_plane(FALSE)
	, m_complete(TRUE)
	, m_thresholdPixel(FALSE)
	, m_pixelThreshold(_T("150"))
{
	mode = ControlModes::ControlObject;

	m_pDoc = nullptr;
	m_2D_dib = nullptr;
	m_2D_frame = nullptr;
	m_3D_frame = nullptr;
	gl_3DTexture = FALSE;

	Act_Rotate = false;
	Act_Translate = false;
	
	scale_x = 0.3F;					
	scale_y = 0.5F;
	scale_z = 0.5F;
	glSlices = 512;
	density = 0.0F;
	intensity = 0.8125F;
	viewDistance = -4.0F;
	
	transY = 0.0F;
	ImageFrame = 1;
	obj_angle = 0.0F;
	pln_angle = 0.0F;

	glVertexPt = New2Dmatrix(64, 3, float);
	lastPos = new float[3]{ 0.0F, 0.0F, 0.0F };
	obj_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	pln_axis = new float[3]{ 0.0F, 0.0F, 0.0F };
	user_Plane = new double[4]{ 1.0L, 0.0L, 0.0L, 1.0L };

	DisplaySlice = 0;
	PixelThreshold = atoi(m_pixelThreshold);
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

	if (m_object != TRUE)
		m_object = TRUE;
	if (m_plane != FALSE)
		m_plane = FALSE;
	if (m_complete != TRUE)
		m_complete = TRUE;
	if (m_thresholdPixel != FALSE)
		m_thresholdPixel = FALSE;

	if (gl_3DTexture != FALSE)
		gl_3DTexture = FALSE;
	if (Act_Translate != false)
		Act_Translate = false;
	if (Act_Rotate != false)
		Act_Rotate = false;
	
	if (m_pixelThreshold.IsEmpty() != true)
		m_pixelThreshold.Empty();

	if (scale_x != 0.3F)
		scale_x = 0.3F;
	if (scale_y != 0.5F)
		scale_y = 0.5F;
	if (scale_z != 0.5F)
		scale_z = 0.5F;
	if (glSlices != 512)
		glSlices = 512;
	if (density != 0.0F)
		density = 0.0F;
	if (intensity != 0.8125F)
		intensity = 0.8125F;
	if (viewDistance != -4.0F)
		viewDistance = -4.0F;

	if (transY != 0.0F)
		transY = 0.0F;
	if (ImageFrame != 1)
		ImageFrame = 1;
	if (obj_angle != 0.0F)
		obj_angle = 0.0F;
	if (pln_angle != 0.0F)
		pln_angle = 0.0F;
	if (Mat_Offset != 0)
		Mat_Offset = 0;

	if (DisplaySlice != 0)
		DisplaySlice = 0;
	if (PixelThreshold != 0)
		PixelThreshold = 0;
	
	glDeleteTextures(5, textureName);
}

void C3DProcess::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SCROLLBAR_2D, m_ScrollBar);

	DDX_Check(pDX, IDC_CHECK_PLANE, m_plane);
	DDX_Check(pDX, IDC_CHECK_Object, m_object);
	DDX_Check(pDX, IDC_CHECK_COMPLETE, m_complete);
	DDX_Check(pDX, IDC_CHECK_PIXEL_THRESHOLD, m_thresholdPixel);

	DDX_Text(pDX, IDC_EDIT_PIXEL_THRES, m_pixelThreshold);
	DDV_MinMaxShort(pDX, atoi(m_pixelThreshold), 0, 255);
	
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

	ON_BN_CLICKED(IDC_CHECK_Object, &C3DProcess::OnBnClickedCheckObject)
	ON_BN_CLICKED(IDC_CHECK_PLANE, &C3DProcess::OnBnClickedCheckPlane)

	ON_EN_CHANGE(IDC_EDIT_PIXEL_THRES, &C3DProcess::OnEnChangeEditPixelThres)
	ON_BN_CLICKED(IDC_CHECK_COMPLETE, &C3DProcess::OnBnClickedCheckComplete)
	ON_BN_CLICKED(IDC_CHECK_PIXEL_THRESHOLD, &C3DProcess::OnBnClickedCheckPixelThreshold)
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
	m_2D_frame->MoveWindow(m_2D_rect.left, m_2D_rect.top, COL, ROW, true);	// 調整物件位置與大小。

	m_3D_frame = GetDlgItem(IDC_STATIC_3D);
	m_3D_frame->GetWindowRect(&m_3D_rect);
	ScreenToClient(&m_3D_rect);
	m_3D_frame->MoveWindow(m_3D_rect.left, m_3D_rect.top, COL, ROW, true);

	m_2D_dib = new CDIB();							
	m_2D_dib->InitDIB(COL, ROW);							// 初始化畫框

	// 設定ScrollBar的範圍
	//
	m_ScrollBar.SetScrollRange(0, Total_Slice-1);

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

	// 在二維影像視窗範圍內
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
	}

	Draw2DImage(DisplaySlice);
	m_ScrollBar.SetScrollPos(DisplaySlice);
	
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
	if (point.x < m_3D_rect.right && point.x > m_3D_rect.left && point.y < m_3D_rect.bottom && point.y > m_3D_rect.top)
	{
		ActStart(nFlags, point.x - m_3D_rect.left, point.y - m_3D_rect.top);

		Draw3DImage(true);
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
	m_thresholdPixel = TRUE;
	UpdateData(FALSE);
	Draw2DImage(DisplaySlice);
}

void C3DProcess::OnEnChangeEditPixelThres()
{
	// 更換 二值化閾值 (Pixel)
	// EditBox : Pixel Threshold (m_pixelThreshold)
	//
	UpdateData(TRUE);
	PixelThreshold = atoi(m_pixelThreshold);
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
	PrepareVolume(textureName);

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

void C3DProcess::PrepareVolume(unsigned int texName[10])
{
	// DO : 建立紋理
	//
	float pixel = 0.0f;
	register int i, j, k;

	// 預備要用來建立紋理的資料矩陣
	//
	int Row = ROW;
	int Col = COL;
	int TotalSlice = Total_Slice;
	int Sample_start = 0 + Mat_Offset;
	int Sample_end = 0 + Mat_Offset + TotalSlice;
	
	BYTE m_image0[256 * 256 * 256][4] = {0};					// 萬惡的佔用記憶體!!!!!!!!!!!!!

	CProgress* m_progress = new CProgress();
	m_progress->Create(IDD_DIALOG_PROGRESSBAR);
	m_progress->ShowWindow(SW_NORMAL);
	m_progress->Set(Total_Slice/2, 0);
	m_progress->SetStatic("Construct 3D Image...");

	i = 0;	j = 0;	k = 0;
	if (m_pDoc->m_img != NULL)
	{
		while (k < 512)
		{
			if (k > Sample_start && k <= Sample_end)
			{
				for (j = 2; j < Row - 2; j += 2)
				{
					for (i = 2; i < Col - 2; i += 2)
					{
						pixel = m_pDoc->m_img[k - (Mat_Offset + 1)][j * Col + i];

						getRamp(m_image0[(i / 2) * 256 * 256 + (j / 2) * 256 + (k / 2)], 
							(float)pixel / 255.0f / 2, 0);
					}
				}
			}
			k += 2;
			m_progress->GetPro(k);
		}
	}
	m_progress->DestroyWindow();
	delete m_progress;

	//--------------------------------------------------------------------------//
	// 建立3D紋理
	//
	if (gl_3DTexture)
	{
		float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };

		// 紋理座標系統：S為橫軸、T為縱軸、R為插入螢幕的軸
		// 設置紋理環繞模式
		//
		glBindTexture(GL_TEXTURE_3D, texName[0]);								// 綁定紋理（指定類型, 紋理對象ID）
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// 紋理過濾函數（選擇濾鏡）
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R_EXT, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);		// 放大時的濾鏡方式
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);		// 縮小時的濾鏡方式
		glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, color);
		glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 256, 256, 256, 0, GL_RGBA,
					GL_UNSIGNED_BYTE, m_image0);								// 創建3D紋理
	}
	
}

void C3DProcess::getRamp(GLubyte* color, float t, int n)
{
	// DO : 計算RGBA的數值
	//
	t *= 2.0f;

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

	glColor4f(1.0f, 1.0f, 1.0f, intensity);				// 設定openGL的slcies顏色（R, G, B, Alpha）

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

	glTranslatef(0.5f, 0.5f, 0.5f);						// 將物件(心臟)移到正中心
	glScalef(scale_x, scale_y, scale_z);				// 將物件(心臟)縮放

	glMultMatrixf(mat);
	glTranslatef(0.0f, 0.0f, -viewDistance);

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

	// 繪製周圍藍色邊框
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

	SwapBuffers(m_hDC);
}

void C3DProcess::Draw2DImage(unsigned short &slice)
{
	//	DO : 繪製 2D 影像
	//
	CClientDC dc(m_2D_frame);

	int Row = ROW;
	int Col = COL;
	register int i;

	if (m_pDoc->m_img != nullptr)
	{
		if (m_complete == TRUE)
		{
			m_2D_dib->ShowInverseDIB(&dc, m_pDoc->m_img[slice]);
		}
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
	}

	// 寫字 (slice)
	//
	CString str;
	str.Format("%d", slice);
	dc.SetTextColor(RGB(255, 255, 0));
	dc.SetBkMode(TRANSPARENT);
	dc.TextOutA(1, 1, str);

}

void C3DProcess::pointToVector(int x, int y, int width, int height, float vec[3])
{
	// DO : project x,y onto a hemi-sphere centered within width, height
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
			viewDistance += 0.01f * (y - transY);
			transY = (float)y;
		}
		else if (mode == ControlModes::ControlPlane)
		{
			user_Plane[3] -= 0.01f * (y - transY);
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


