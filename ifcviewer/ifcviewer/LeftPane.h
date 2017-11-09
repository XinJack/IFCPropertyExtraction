#pragma once


#include	"GenericTreeItem.h"


// CLeftPane form view

class CLeftPane : public CTreeView
{
//	DECLARE_DYNCREATE(CLeftPane)

protected:
	CLeftPane(CWnd* pParent = NULL);           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CLeftPane);
	virtual ~CLeftPane();

public:
	enum { IDD = IDD_LEFTPANE };
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

public:
	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual void BuildTreeItem(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, bool disableItems);
	virtual void Update(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, int32_t select);
	virtual void BuildPropertySetTreeItem(STRUCT__SELECTABLE__TREEITEM * selectableTreeItem);
	virtual void BuildHeaderSetTreeItem(STRUCT__HEADER__SET * headerSet, HTREEITEM parentHTreeItem);
	virtual void BuildPropertyTreeItem(STRUCT__PROPERTY__SET * ifcPropertySet);

	afx_msg HTREEITEM SelectTreeItem(STRUCT__IFC__OBJECT * ifcObject, HTREEITEM hItem);
	afx_msg void UpdateSelect(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, int_t * pUnchecked, int_t * pChecked);
	afx_msg void OnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeforeExpand(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnAttrExtract(); // 属性提取函数定义

	DECLARE_MESSAGE_MAP()
};
