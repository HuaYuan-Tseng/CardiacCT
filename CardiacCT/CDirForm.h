
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
	CCTDoc*		m_pDoc;						// 紀錄Doc內容的物件指標
	CListCtrl	m_SerList;					// 選擇時序的列表
	CListCtrl	m_ContList;					// 顯示DICOMDIR內容的列表

	int			m_clickItem;				// SeriesList被點擊的項目
	int			m_checkItem;				// SeriesList被勾選的項目
	bool		m_bHit;


//================//
//   Operations   //
//================//
public:
	void displayContextList(int &series);	// 顯示指定時序下的DICOMDIR內容


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
