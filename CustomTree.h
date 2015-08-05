#pragma once
#include "Modules.h"
#include "PestRid.h"

////////////////////////////////////////////////////////////////////////////////
// CCustomTreeView

class CCustomTreeCtrl;

class CCustomTreeView : public CTreeView
{
// Construction
public:
	CCustomTreeView();

public:
	BOOL DoQuery()
	{
		POSITION pos;
		DisplaySet* pSet;
		CEnumModel* InfoClass;
		BOOL bUpdate = FALSE;
		//in case user adds/removes top level (or lower?) items from the tree
		if (m_TreeUpdateCS.Lock()) {
			pos = m_TopLevelTreeUpdaters.GetStartPosition();
			while (pos) {
				m_TopLevelTreeUpdaters.GetNextAssoc(pos, (void*&)pSet,
													(void*&)InfoClass);
				if (InfoClass) {
					InfoClass->OnUpdateDatabase();
					bUpdate = TRUE;
				}
			}
			m_TreeUpdateCS.Unlock();
		}
		return bUpdate;
	}
	DisplaySet* GetItemType(HTREEITEM hItem)
	{
		if (hItem == NULL || hItem == TVI_ROOT) return NULL;
		while (GetTreeCtrl().GetParentItem(hItem) != NULL) {
			hItem = GetTreeCtrl().GetParentItem(hItem);
		}
		return (DisplaySet*)GetTreeCtrl().GetItemData(hItem);
	}
	CEnumModel* GetItemModel(DisplaySet* pSet)
	{
		CEnumModel* pInfoClass;
		return (m_TopLevelTreeUpdaters.Lookup(pSet,
			(void*&)pInfoClass)) ? pInfoClass : NULL;
	}
	void* GetItemParam(HTREEITEM hItem, LPARAM lParam)
	{
		return (void*)(GetTreeCtrl().GetParentItem(hItem) == NULL ? NULL : lParam);
	}
protected:
	BOOL PreCreateWindow(CREATESTRUCT & cs)
	{
		cs.style =	TVS_HASBUTTONS | TVS_HASLINES |
					TVS_LINESATROOT | TVS_SHOWSELALWAYS |
					WS_TABSTOP | WS_CHILD | WS_VISIBLE;
		return CTreeView::PreCreateWindow(cs);
	}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		DWORD dwCount;
		int iRet = 0;
		CBitmap Bitmap;
		if (CTreeView::OnCreate(lpCreateStruct) == -1) return -1;
		dwCount = 0;
		while (DisplaySets[dwCount].pTreeRenderer.pRenderer) {
			//if (DisplaySets[dwCount].bDisplay) {
				if (m_TreeUpdateCS.Lock()) {
					m_DisplaySets.Add(&DisplaySets[dwCount]);
					m_TopLevelTreeUpdaters.SetAt(&DisplaySets[dwCount],
						new CEnumModel(&DisplaySets[dwCount], TRUE));
					m_TreeUpdateCS.Unlock();
				}
			//}
			dwCount++;
		}
		if (!m_TreeImageList.Create(16, 16, ILC_COLOR, 2, 10)) {
			iRet = -1;
		}
		if (iRet != -1 && Bitmap.LoadBitmap(IDB_DIR)) {
			if (m_TreeImageList.Add(&Bitmap, RGB(0, 0, 0)) == -1) {
				iRet = -1;
			}
			Bitmap.DeleteObject();
		}
		if (iRet != -1 && Bitmap.LoadBitmap(IDB_DIRSEL)) {
			if (m_TreeImageList.Add(&Bitmap, RGB(0, 0, 0)) == -1) {
				iRet = -1;
			}
			Bitmap.DeleteObject();
		}
		GetTreeCtrl().SetImageList(&m_TreeImageList, TVSIL_NORMAL);
		if (iRet == -1) OnDestroy();
		return iRet;
	}
	afx_msg void OnDestroy()
	{
		POSITION pos;
		DisplaySet* pSet;
		CEnumModel* InfoClass;
		GetTreeCtrl().DeleteAllItems();
		GetTreeCtrl().SetImageList(NULL, TVSIL_NORMAL);
		m_TreeImageList.DeleteImageList();
		if (m_TreeUpdateCS.Lock()) {
			pos = m_TopLevelTreeUpdaters.GetStartPosition();
			while (pos) {
				m_TopLevelTreeUpdaters.GetNextAssoc(pos, (void*&)pSet,
													(void*&)InfoClass);
				delete InfoClass;
			}
			m_TopLevelTreeUpdaters.RemoveAll();
			m_TreeUpdateCS.Unlock();
		}
		CTreeView::OnDestroy();
	}
	struct ViewChangeDescription {
		int iType; //0 for insertion, 1 for deletion, 2 for update
		HTREEITEM hItem; //item or parent item for insertions
		LPARAM lParam;
	};
	afx_msg LRESULT OnUpdateUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		int iCount;
		HTREEITEM hItem;
		DWORD dwCount;
		CEnumModel* InfoClass;
		CArray<TreeViewChange*>* ViewChange;
		for (dwCount = 0; dwCount < (DWORD)m_DisplaySets.GetCount(); dwCount++) {
			m_TopLevelTreeUpdaters.Lookup(m_DisplaySets[dwCount],
												(void*&)InfoClass);
			if (InfoClass &&
				(ViewChange = InfoClass->GetViewChanges()) != NULL) {
				if ((hItem = GetRootItemBylParam((LPARAM)m_DisplaySets[dwCount])) == NULL) {
					hItem = TVI_ROOT; //root item not yet inserted
				}
				CArray<ViewChangeDescription, ViewChangeDescription&> ViewChangeDescs;
				for (iCount = 0; iCount < ViewChange->GetCount(); iCount++) {
					UpdateTreeViewChange(ViewChange->GetAt(iCount), hItem, ViewChangeDescs);
				}
				InfoClass->OnUpdateUI();
				for (iCount = 0; iCount < ViewChangeDescs.GetCount(); iCount++) {
					if (ViewChangeDescs[iCount].iType == 0) {
						TVINSERTSTRUCT tvisNew = { ViewChangeDescs[iCount].hItem, TVI_LAST, 
										{	TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE |
											TVIF_SELECTEDIMAGE | TVIF_PARAM, NULL,
											0, 0,//future: TVIS_EXPANDED/EXPANDEDONCE lookup
											LPSTR_TEXTCALLBACK, 0,
											I_IMAGECALLBACK, I_IMAGECALLBACK,
											I_CHILDRENCALLBACK, ViewChangeDescs[iCount].hItem == TVI_ROOT ?
																(LPARAM)m_DisplaySets[dwCount] :
																ViewChangeDescs[iCount].lParam } };
						GetTreeCtrl().InsertItem(&tvisNew); //log error
					} else if (ViewChangeDescs[iCount].iType == 1) {
						GetTreeCtrl().DeleteItem(ViewChangeDescs[iCount].hItem);
					} else if (ViewChangeDescs[iCount].iType == 2) {
						TVITEM tviNew = {	TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE |
											TVIF_SELECTEDIMAGE | TVIF_PARAM, ViewChangeDescs[iCount].hItem,
											0, 0,//future: TVIS_EXPANDED/EXPANDEDONCE lookup
											LPSTR_TEXTCALLBACK, 0,
											I_IMAGECALLBACK, I_IMAGECALLBACK,
											I_CHILDRENCALLBACK, ViewChangeDescs[iCount].lParam };
						GetTreeCtrl().SetItem(&tviNew); //log error
					}
				}
			}
			if ((hItem = GetRootItemBylParam((LPARAM)m_DisplaySets[dwCount])) == NULL) {
				if (m_DisplaySets[dwCount]->pTreeRenderer.paGathererSets[0] != NULL &&
					m_DisplaySets[dwCount]->pTreeRenderer.dwType == 0) {
				} else {
					TVINSERTSTRUCT tvisNew = { TVI_ROOT, TVI_LAST, 
									{	TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE |
										TVIF_SELECTEDIMAGE | TVIF_PARAM, NULL,
										0, 0,//future: TVIS_EXPANDED/EXPANDEDONCE lookup
										LPSTR_TEXTCALLBACK, 0,
										I_IMAGECALLBACK, I_IMAGECALLBACK,
										I_CHILDRENCALLBACK, (LPARAM)m_DisplaySets[dwCount] } };
					if ((hItem = GetTreeCtrl().InsertItem(&tvisNew)) != NULL) {
						if (GetTreeCtrl().GetCount() == 1 &&
							GetTreeCtrl().GetSelectedItem() == NULL) {
							GetTreeCtrl().SetItemState(hItem, TVIS_SELECTED, TVIS_SELECTED);
							NMTREEVIEW nmtv;
							LRESULT lResult;
							nmtv.itemNew.hItem = hItem;
							OnTvnSelChanged((NMHDR*)&nmtv, &lResult);
						}
					}
				}
			}
		}
		return 0;
	}
	afx_msg void OnTvnSelChanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnItemExpanding(NMHDR *pNMHDR, LRESULT *pResult)
	{
		DisplaySet* pSet = (DisplaySet*)GetItemType(((LPNMTREEVIEW)pNMHDR)->itemNew.hItem);
		CEnumModel* pModel = GetItemModel(pSet);
		DWORD dwCount;
		void* pvKey = GetItemParam(((LPNMTREEVIEW)pNMHDR)->itemNew.hItem, ((LPNMTREEVIEW)pNMHDR)->itemNew.lParam);
		//update tree item for expanded/expandedonce
		if (((LPNMTREEVIEW)pNMHDR)->action == TVE_TOGGLE) {
		} else if (((LPNMTREEVIEW)pNMHDR)->action == TVE_EXPAND &&
					GetTreeCtrl().GetChildItem(((LPNMTREEVIEW)pNMHDR)->itemNew.hItem) == NULL) {
			for (dwCount = 0; dwCount < (DWORD)pModel->GetChildCount(pvKey); dwCount++) {
				TVINSERTSTRUCT tvisNew = { ((LPNMTREEVIEW)pNMHDR)->itemNew.hItem, TVI_LAST, 
								{	TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE |
									TVIF_SELECTEDIMAGE | TVIF_PARAM, NULL,
									0, 0,//future: TVIS_EXPANDED/EXPANDEDONCE lookup
									LPSTR_TEXTCALLBACK, 0,
									I_IMAGECALLBACK, I_IMAGECALLBACK,
									I_CHILDRENCALLBACK, (LPARAM)pModel->GetChildParam(pvKey, dwCount) } };
				GetTreeCtrl().InsertItem(&tvisNew); //log error
			}
			if (((LPNMTREEVIEW)pNMHDR)->itemNew.lParam != (LPARAM)pSet) {
				for (dwCount = 0; dwCount < (DWORD)m_DisplaySets.GetCount(); dwCount++) {
					if (m_DisplaySets[dwCount]->pTreeRenderer.paGathererSets[0] != NULL &&
						m_DisplaySets[dwCount]->pTreeRenderer.dwType == 0) {
						CGatherer* pGatherer = (CGatherer*)m_DisplaySets[dwCount]->pTreeRenderer.paGathererSets[0]->CreateObject();
						CArray<CRuntimeClass*> Dependencies;
						pGatherer->GetDependencies(Dependencies);
						delete pGatherer;
						if (Dependencies[0] ==
							pSet->pTreeRenderer.paGathererSets[0]) {
							TVINSERTSTRUCT tvisNew = { ((LPNMTREEVIEW)pNMHDR)->itemNew.hItem, TVI_LAST, 
											{	TVIF_CHILDREN | TVIF_TEXT | TVIF_IMAGE |
												TVIF_SELECTEDIMAGE | TVIF_PARAM, NULL,
												0, 0,//future: TVIS_EXPANDED/EXPANDEDONCE lookup
												LPSTR_TEXTCALLBACK, 0,
												I_IMAGECALLBACK, I_IMAGECALLBACK,
												I_CHILDRENCALLBACK, (LPARAM)m_DisplaySets[dwCount] } };
							GetTreeCtrl().InsertItem(&tvisNew);
						}
					}
				}
			}
		} else if (((LPNMTREEVIEW)pNMHDR)->action == TVE_COLLAPSE) {
			/*HTREEITEM hNextItem;
			HTREEITEM hChildItem = GetTreeCtrl().GetChildItem(((LPNMTREEVIEW)pNMHDR)->itemNew.hItem);
			while (hChildItem != NULL) {
				hNextItem = GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
				GetTreeCtrl().DeleteItem(hChildItem);
				hChildItem = hNextItem;
			}*/
		} else if (	((LPNMTREEVIEW)pNMHDR)->action ==
					(TVE_COLLAPSE | TVE_COLLAPSERESET)) {
		}
		*pResult = 0;
	}
	afx_msg void OnTvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
	{
		CString Str;
		LPNMTVDISPINFO pDispInfo = (LPNMTVDISPINFO)pNMHDR;
		DisplaySet* pSet;
		CEnumModel* pInfoClass;
		if (m_TopLevelTreeUpdaters.Lookup(((pDispInfo->item.mask & TVIF_PARAM) ?
						(void*)pDispInfo->item.lParam : 
						(void*)GetTreeCtrl().GetItemData(pDispInfo->item.hItem)),
							(void*&)pInfoClass)) {
			pSet = (DisplaySet*)((pDispInfo->item.mask & TVIF_PARAM) ?
						pDispInfo->item.lParam : 
						GetTreeCtrl().GetItemData(pDispInfo->item.hItem));
		} else {
			pSet = GetItemType(pDispInfo->item.hItem);
			pInfoClass = GetItemModel(pSet);
		}
		void* pvKey =	GetItemParam(pDispInfo->item.hItem, ((pDispInfo->item.mask & TVIF_PARAM) ?
						pDispInfo->item.lParam : 
						GetTreeCtrl().GetItemData(pDispInfo->item.hItem)));
		if (pDispInfo->item.mask & TVIF_TEXT) {
			pInfoClass->Render(&pSet->pTreeRenderer, pvKey, &Str);
			_tcsncpy(pDispInfo->item.pszText, Str, pDispInfo->item.cchTextMax);
			pDispInfo->item.pszText[pDispInfo->item.cchTextMax - 1] = 0;
		}
		if (pDispInfo->item.mask & TVIF_IMAGE) {
			pDispInfo->item.iImage = 0;
		}
		if (pDispInfo->item.mask & TVIF_SELECTEDIMAGE) {
			pDispInfo->item.iSelectedImage = 1;
		}
		if (pDispInfo->item.mask & TVIF_CHILDREN) {
			BOOL bChildren = FALSE;
			DWORD dwCount;
			for (dwCount = 0; dwCount < (DWORD)m_DisplaySets.GetCount(); dwCount++) {
				if (m_DisplaySets[dwCount]->pTreeRenderer.paGathererSets[0] != NULL &&
					m_DisplaySets[dwCount]->pTreeRenderer.dwType == 0) {
					CGatherer* pGatherer = (CGatherer*)m_DisplaySets[dwCount]->pTreeRenderer.paGathererSets[0]->CreateObject();
					CArray<CRuntimeClass*> Dependencies;
					pGatherer->GetDependencies(Dependencies);
					delete pGatherer;
					if (Dependencies[0] ==
						pSet->pTreeRenderer.paGathererSets[0]) {
						bChildren = TRUE;
					}
				}
			}
			pDispInfo->item.cChildren =
					pInfoClass->GetChildCount(pvKey) == 0 && !bChildren ? 0 : 1;
		}
		*pResult = 0;
	}
	void UpdateTreeViewChange(	TreeViewChange* ViewChange,
								HTREEITEM hParentItem,
								CArray<ViewChangeDescription, ViewChangeDescription&> & ViewChangeDescs)
	{
		POSITION pos;
		HTREEITEM hItem;
		void* pvKey;
		ViewChangeDescription NewViewChangeDesc;
		ASSERT(ViewChange);
		TreeViewChange* NextViewChange;
		hItem = GetTreeCtrl().GetChildItem(hParentItem);
		while (hItem) {
			if ((void*)GetTreeCtrl().GetItemData(hItem) == ViewChange->pvOldKey) {
				break;
			}
			hItem = GetTreeCtrl().GetNextSiblingItem(hItem);
		}
		if (ViewChange->pvOldKey == ViewChange->pvNewKey) {
			if (ViewChange->pvOldKey == (void*)~0) {
				//just process underlying list
			} else {
				//no change nothing to do
			}
		} else if (ViewChange->pvOldKey == (void*)~0) {
			//insert new item
			if (hItem) {
			}
			NewViewChangeDesc.iType = 0;
			NewViewChangeDesc.lParam = (LPARAM)ViewChange->pvNewKey;
			NewViewChangeDesc.hItem = hParentItem;
			ViewChangeDescs.Add(NewViewChangeDesc);
		} else if (ViewChange->pvNewKey == (void*)~0) {
			//delete stale item
			if (hItem) {
				NewViewChangeDesc.iType = 1;
				NewViewChangeDesc.hItem = hItem;
				ViewChangeDescs.Add(NewViewChangeDesc);
			}
		} else {
			//update item
			if (hItem) {
				NewViewChangeDesc.iType = 2;
				NewViewChangeDesc.lParam = (LPARAM)ViewChange->pvNewKey;
				NewViewChangeDesc.hItem = hItem;
				ViewChangeDescs.Add(NewViewChangeDesc);
				pos = ViewChange->ChildMap.GetStartPosition();
				while (pos) {
					ViewChange->ChildMap.GetNextAssoc(pos, pvKey,
															(void*&)NextViewChange);
					UpdateTreeViewChange(	NextViewChange,
											hItem,
											ViewChangeDescs);
				}
			}
		}
		//must pre-sort inserted items so not in current n^2 manner...
		//TVSORTCB tvscb = { hItem, TreeCompareFunc, (LPARAM)this };
		//GetTreeCtrl().SortChildrenCB(&tvscb);
	}
	static int CALLBACK TreeCompareFunc(LPARAM /*lParam1*/,
										LPARAM /*lParam2*/,
										LPARAM /*lParamSort*/)
	{
		return 0;
	}
	HTREEITEM GetItemBylParam(HTREEITEM hParentItem, LPARAM lParam)
	{
		HTREEITEM hItem;
		hItem = GetTreeCtrl().GetChildItem(hParentItem);
		while (hItem) {
			if (GetTreeCtrl().GetItemData(hItem) == (DWORD_PTR)lParam) {
				break;
			}
			hItem = GetTreeCtrl().GetNextSiblingItem(hItem);
		}
		return hItem;
	}
	HTREEITEM GetRootItemBylParam(LPARAM lParam)
	{
		return GetItemBylParam(TVI_ROOT, lParam);
	}
	CMapPtrToPtr m_TopLevelTreeUpdaters;
	CArray<DisplaySet*> m_DisplaySets; 
	CImageList m_TreeImageList;
	CCriticalSection m_TreeUpdateCS;

	DECLARE_DYNCREATE(CCustomTreeView)
	DECLARE_MESSAGE_MAP()
};