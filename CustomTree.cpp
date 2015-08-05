#include "stdafx.h"
#include "PestRid.h"
#include "PestRidDlg.h"
#include "CustomTree.h"

////////////////////////////////////////////////////////////////////////////////
// CCustomTreeView

IMPLEMENT_DYNCREATE(CCustomTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CCustomTreeView, CTreeView)
//{{AFX_MSG_MAP(CCustomTreeView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_PESTRID_UPDATEUI, OnUpdateUI)
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnTvnGetDispInfo)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnTvnSelChanged)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemExpanding)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

afx_msg void CCustomTreeView::OnTvnSelChanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	DisplaySet* pSet;
	CEnumModel* pInfoClass;
	void* pvKey;
	pSet = GetItemType(((NMTREEVIEW*)pNMHDR)->itemNew.hItem);
	if (m_TopLevelTreeUpdaters.Lookup((void*)GetTreeCtrl().GetItemData(((NMTREEVIEW*)pNMHDR)->itemNew.hItem),
						(void*&)pInfoClass)) {
		pInfoClass = GetItemModel(pSet);
		pSet = (DisplaySet*)GetTreeCtrl().GetItemData(((NMTREEVIEW*)pNMHDR)->itemNew.hItem);
	} else
		pInfoClass = GetItemModel(pSet);
	if (GetTreeCtrl().GetParentItem(((NMTREEVIEW*)pNMHDR)->itemNew.hItem)) {
		pvKey = GetItemParam(GetTreeCtrl().GetParentItem(((NMTREEVIEW*)pNMHDR)->itemNew.hItem), GetTreeCtrl().GetItemData(GetTreeCtrl().GetParentItem(((NMTREEVIEW*)pNMHDR)->itemNew.hItem)));
	} else {
		pvKey = GetItemParam(((NMTREEVIEW*)pNMHDR)->itemNew.hItem, GetTreeCtrl().GetItemData(((NMTREEVIEW*)pNMHDR)->itemNew.hItem));
	}
	((CPestRidSplitterWnd*)GetParent())->OnSelChange(
			pInfoClass, pSet, pvKey);
	*pResult = 0;
}

inline CCustomTreeView::CCustomTreeView() : CTreeView() { }