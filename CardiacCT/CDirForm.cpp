
// CDirForm.cpp : implementation file
//

#include "pch.h"
#include "CardiacCT.h"
#include "afxdialogex.h"
#include "CDirForm.h"
#include "C3DProcess.h"
#include <vector>
using namespace std;

//===================================//
//   DICOMDIR Contents Form Dialog   //
//===================================//

IMPLEMENT_DYNAMIC(CDirForm, CDialogEx)

CDirForm::CDirForm(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_DIRCONTENTS, pParent)
{
	m_pDoc = nullptr;		// �u���l�ơA���藍��delete!!
	m_clickItem = 0;
	m_checkItem = 0;
}

CDirForm::~CDirForm()
{
	
}

void CDirForm::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_SerList);
	DDX_Control(pDX, IDC_LIST3, m_ContList);
}

BEGIN_MESSAGE_MAP(CDirForm, CDialogEx)
	ON_BN_CLICKED(IDOK, &CDirForm::OnBnClickedOk)
	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CDirForm::OnClickSeriesList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CDirForm::OnItemChangedList1)
END_MESSAGE_MAP()

//===============================//
//   CDirForm message handlers   //
//===============================//

BOOL CDirForm::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	// DO : Dialog ��l��
	// 

	// ��Dialog���D
	//
	SetWindowText(m_pDoc->m_dir->DirPathName);

	// Series List �K�[�C�����u
	//
	DWORD dwStyle = m_SerList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	dwStyle |= LVS_EX_CHECKBOXES;
	dwStyle |= LVS_EX_GRIDLINES;
	m_SerList.SetExtendedStyle(dwStyle);

	// Series List �K�[���C���D
	//
	m_SerList.InsertColumn(0, "Series", LVCFMT_LEFT, 100);
	m_SerList.InsertColumn(1, "Series Number", LVCFMT_LEFT, 110);

	// Series List ��l�Ƥ��e(Ū����Series)
	//
	int n = 0;
	vector<DcmSeries*>::iterator iter;
	for (iter = m_pDoc->m_dir->SeriesList.begin(); 
		iter != m_pDoc->m_dir->SeriesList.end();	++iter)
	{
		CString str_series;
		CString str_seriesNum;

		str_series.Format(_T("%d"), n);
		str_seriesNum = (*(iter))->SeriesNumber;

		m_SerList.InsertItem(n, _T(""));
		m_SerList.SetItemText(n, 0, str_series);
		m_SerList.SetItemText(n++, 1, str_seriesNum);
	}

	//------------------------------------------------------------//

	// Context List �K�[�C�����u
	//
	dwStyle = m_ContList.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;
	dwStyle |= LVS_EX_GRIDLINES;
	m_ContList.SetExtendedStyle(dwStyle);

	// Context List �K�[���C���D
	//
	m_ContList.InsertColumn(0, "Description", LVCFMT_LEFT, 250);
	m_ContList.InsertColumn(1, "Value", LVCFMT_LEFT, 250);

	// �w�]����ܲ�0�ɧ�(�ΰߤ@�����Ӯɧ�)�����e
	//
	m_SerList.SetCheck(m_checkItem, TRUE);
	displayContextList(m_clickItem);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDirForm::OnClickSeriesList(NMHDR *pNMHDR, LRESULT *pResult)
{
	// ID : IDC_LIST1 (m_SerList)
	// DO : �I��Series List�W���ɧǡA��o�I���B���ɧ�(�渹)���[�ݸӮɧǤ��e
	//

	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: Add your control notification handler code here

	// ��o�ƹ���m
	//
	DWORD dwPos = GetMessagePos();
	CPoint point(LOWORD(dwPos), HIWORD(dwPos));
	m_SerList.ScreenToClient(&point);

	// �w�q���c��
	//
	// typedef struct _LVHITTESTINFO {		// �Ω�HitTest()�եΪ����c��
	//	 POINT	pt;							// �Ĥ@�Ӱ�����J
	//	 UINT	flags;						// ��L�T�Ӭ���X
	//	 int	iItem;
	//	 int	iSubItem;
	// }	LVHITTESTINFO,	*LPLVHITTESTINFO;
	LVHITTESTINFO lvInfo;
	lvInfo.pt = point;
	lvInfo.flags = LVHT_ABOVE;

	// ��o�ɧ�(�渹)
	//
	int nItem = m_SerList.SubItemHitTest(&lvInfo);
	if (nItem != -1)
	{
		m_clickItem = lvInfo.iItem;
		displayContextList(m_clickItem);
	}

	// �P�_�O�_�I���bcheckBox�W
	//
	if (lvInfo.flags == LVHT_ONITEMSTATEICON)
	{
		m_bHit = true;
		m_checkItem = m_clickItem;
	}

	//TRACE1("Item %d was selected\n", m_clickItem);
	//TRACE1("Item %d's checkBox was selected\n", m_checkItem);

	*pResult = 0;
}

void CDirForm::OnItemChangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	// ID : IDC_LIST1 (m_SerList)
	// DO : �P�_Series List��checkBox�Ŀ窱�A
	//

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here

	if (m_bHit)
	{
		m_bHit = false;

		if (m_SerList.GetCheck(m_checkItem))
		{
			for (int n = 0; n <= 9; n++)
			{
				if (n != m_checkItem && m_SerList.GetCheck(n) == TRUE)
					m_SerList.SetCheck(n, FALSE);
			}
		}
	}

	*pResult = 0;
}

void CDirForm::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	m_pDoc->displaySeries = m_checkItem;	// �^�ǤĿ�n��ܪ��ɧ�

	CDialogEx::OnOK();
}

//========================//
//   CDirForm Functions   //
//========================//

void CDirForm::displayContextList(int &series)
{
	// DO : ��ܬY�@�ɧǤ��e
	//

	// �M���C��Ҧ����e
	//
	m_ContList.DeleteAllItems();

	// Context List ��ܤ��e(Series�����e)
	//
	int n = 0;
	CString temp;

	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Patient Name");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->PatientName);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Patient ID");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->PatientID);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Study ID");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->StudyID);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Rescale Intercept");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Rescale_Intercept);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Rescale Slope");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Rescale_Slope);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Rescale Type");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Rescale_Type);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Window Center & Width Explanation");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Window_Explanation);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Window Center");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Window_Center);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Window Width");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Window_Width);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Spacing Between Slices");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Spacing_Between_Slices);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Slice Thickness");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Slice_Thickness);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Voxel Spacing (mm)");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Voxel_Spacing);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Series Number");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->SeriesList[series]->SeriesNumber);

	n = n + 1;
	temp.Format("%d", m_pDoc->m_dir->SeriesList[series]->TotalSliceCount);
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Total Slice");
	m_ContList.SetItemText(n, 1, temp);

	n = n + 1;
	temp.Format("%d", m_pDoc->m_dir->Row);
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Row");
	m_ContList.SetItemText(n, 1, temp);

	n = n + 1;
	temp.Format("%d", m_pDoc->m_dir->Col);
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Col");
	m_ContList.SetItemText(n, 1, temp);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Pixel Representation");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Pixel_Representation);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Bits Allocated");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Bits_Allocated);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "Bits Stored");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Bits_Stored);

	n = n + 1;
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "High Bit");
	m_ContList.SetItemText(n, 1, m_pDoc->m_dir->Bits_HiBit);

	n = n + 1;
	temp.Format("%d", m_pDoc->m_dir->HU_min);
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "HU min");
	m_ContList.SetItemText(n, 1, temp);

	n = n + 1;
	temp.Format("%d", m_pDoc->m_dir->HU_max);
	m_ContList.InsertItem(n, _T(""));
	m_ContList.SetItemText(n, 0, "HU max");
	m_ContList.SetItemText(n, 1, temp);

}


