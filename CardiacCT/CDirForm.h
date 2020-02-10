
// CDirForm.h : interface of the CDirForm class
//

//===================================//
//   DICOMDIR Contents Form Dialog   //
//===================================//

#pragma once
#include "CCTDoc.h"

class CDirForm : public CDialogEx
{
	DECLARE_DYNAMIC(CDirForm)

//================//
//   Attributes   //
//================//
public:
	CCTDoc*		m_pDoc;						// ����Doc���e���������
	CListCtrl	m_SerList;					// ��ܮɧǪ��C��
	CListCtrl	m_ContList;					// ���DICOMDIR���e���C��

	int			m_clickItem;				// SeriesList�Q�I��������
	int			m_checkItem;				// SeriesList�Q�Ŀ諸����
	bool		m_bHit;


//================//
//   Operations   //
//================//
public:
	void displayContextList(int &series);	// ��ܫ��w�ɧǤU��DICOMDIR���e


//================//
// Implementation //
//================//
public:
	CDirForm(CWnd* pParent = nullptr);		// standard constructor
	virtual ~CDirForm();
	virtual BOOL OnInitDialog();

//=================//
//   Dialog Data   //
//=================//
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_DIRCONTENTS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClickSeriesList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnItemChangedList1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedOk();
};
