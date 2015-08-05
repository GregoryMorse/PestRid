#include "stdafx.h"
#include "PestRid.h"
#include "PestRidDlg.h"
#include "CustomListCtrl.h"

////////////////////////////////////////////////////////////////////////////////
// CColumnMap

int __cdecl CColumnMap::OrderCompare(const void* pCol1, const void* pCol2)
{
	if (((ColumnMap*)pCol1)->dwColGroup == ((ColumnMap*)pCol2)->dwColGroup) {
		if (((ColumnMap*)pCol1)->dwOrder == ((ColumnMap*)pCol2)->dwOrder) {
			return 0;
		}
		return (((ColumnMap*)pCol1)->dwOrder > 
				((ColumnMap*)pCol2)->dwOrder) ? 1 : -1;
	}
	return (((ColumnMap*)pCol1)->dwColGroup >
			((ColumnMap*)pCol2)->dwColGroup) ? 1 : -1;
}

int __cdecl CColumnMap::SortOrderCompare(const void* pCol1, const void* pCol2)
{
	if ((*(ColumnMap**)pCol1)->dwColGroup == (*(ColumnMap**)pCol2)->dwColGroup) {
		if ((*(ColumnMap**)pCol1)->dwOrder == (*(ColumnMap**)pCol2)->dwOrder) {
			return 0; //could break tie on other column criteria
		}
		return ((*(ColumnMap**)pCol1)->dwSortOrder > 
				(*(ColumnMap**)pCol2)->dwSortOrder) ? 1 : -1;
	}
	return ((*(ColumnMap**)pCol1)->dwColGroup >
			(*(ColumnMap**)pCol2)->dwColGroup) ? 1 : -1;
}

////////////////////////////////////////////////////////////////////////////////
// CCustomHeaderCtrl

BEGIN_MESSAGE_MAP(CCustomHeaderCtrl, CHeaderCtrl)
//{{AFX_MSG_MAP(CCustomHeaderCtrl)
	ON_NOTIFY_REFLECT(HDN_BEGINTRACK, OnBeginTrack)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnHdnCustomDrawCustomHeader)
	ON_NOTIFY_REFLECT(HDN_GETDISPINFO, OnHdnGetDispInfoCustomHeader)
	ON_NOTIFY_REFLECT(HDN_DIVIDERDBLCLICK, OnHdnDividerDblClick)
	ON_WM_SETCURSOR()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CEnumModel* CCustomHeaderCtrl::GetCurrentModel()
{ return ((CCustomListView*)GetParent())->GetCurrentModel(); }

INT_PTR CCustomHeaderCtrl::GetColGroupIndex()
{ return ((CCustomListView*)GetParent())->GetColGroupIndex(); }

CColumnMap* CCustomHeaderCtrl::GetColumnMap()
{ return ((CCustomListView*)GetParent())->GetColumnMap(); }

DisplaySet* CCustomHeaderCtrl::GetDisplaySet()
{ return ((CCustomListView*)GetParent())->GetDisplaySet();
}

afx_msg void CCustomHeaderCtrl::OnHdnDividerDblClick(NMHDR* pNMHDR, LRESULT *pResult)
{
	if (((NMHEADER*)pNMHDR)->iButton == 0)
		((CCustomListView*)GetParent())->AutoSetColumnWidth(GetColumnMap()->GetColumnFromIndex(((NMHEADER*)pNMHDR)->iItem, (DWORD)GetColGroupIndex()));
}


////////////////////////////////////////////////////////////////////////////////
// CCustomListView

int CCustomListView::Init()
{
	DWORD dwCount;
	DWORD dwTotalCols;
	CHeaderCtrl* pHeaderCtrl = GetListCtrl().GetHeaderCtrl();
/*	pHeaderCtrl->SendMessage(CCM_SETUNICODEFORMAT,
#ifdef _UNICODE
		TRUE
#else
		FALSE
#endif
		, 0);*/
	//first column autosize takes up whole width if sizing while inserting
	ShowWindow(SW_HIDE);
	GetListCtrl().SetImageList(NULL, LVSIL_NORMAL);
	GetListCtrl().SetImageList(NULL, LVSIL_SMALL);
	GetListCtrl().DeleteAllItems();
	if (pHeaderCtrl) {
		dwTotalCols = pHeaderCtrl->GetItemCount();
		for (dwCount = dwTotalCols - 1; dwCount != ~0UL; dwCount--) {
			GetListCtrl().DeleteColumn(dwCount);
		}
	}
	for (dwCount = 0; dwCount < GetColumnMap()->GetColumnCount(); dwCount++) {
		if (GetColumnMap()->GetColumnSelected(dwCount) &&
			GetColumnMap()->GetColumnGroup(dwCount) ==
			(DWORD)GetColGroupIndex()) {
			InsertHeaderColumn(dwCount);
		}
	}
	return 0;
}

IMPLEMENT_DYNCREATE(CCustomListView, CListView)

//vertical scrolling messages are reflected
BEGIN_MESSAGE_MAP(CCustomListView, CListView)
//{{AFX_MSG_MAP(CCustomListView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	ON_WM_VSCROLL()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnLvnCustomDrawCustomList)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetDispInfoCustomList)
	//LVN_ODCACHEHINT should implement for speed improvement
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM, OnLvnOdFindItem)
	ON_NOTIFY_REFLECT(NM_CLICK, OnLvnClickCustomList)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnLvnRClickCustomList)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClickCustomList)
	ON_NOTIFY_REFLECT(LVN_MARQUEEBEGIN, OnLvnMarqueeBegin)
	ON_NOTIFY_REFLECT(NM_RELEASEDCAPTURE, OnLvnReleaseCapture)
	ON_NOTIFY_REFLECT(LVN_ENDSCROLL, OnLvnEndScroll)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnLvnItemChanged)
	ON_WM_SIZE()
	ON_WM_NCCALCSIZE()
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#define LVIS_ALL (LVIS_FOCUSED | LVIS_SELECTED | LVIS_CUT | LVIS_DROPHILITED | LVIS_GLOW | LVIS_ACTIVATING)
#define LVFI_ALL (LVFI_PARTIAL | LVFI_SUBSTRING | LVFI_STRING | LVFI_WRAP)

afx_msg void CCustomListView::OnLvnMarqueeBegin(NMHDR* pNMHDR, LRESULT* plResult)
{
	((CCustomMultiListView*)GetParent())->SetMarquee(GetSafeHwnd());
}

afx_msg void CCustomListView::OnLvnReleaseCapture(NMHDR* pNMHDR, LRESULT* plResult)
{
	((CCustomMultiListView*)GetParent())->SetMarquee(NULL);
}

afx_msg void CCustomListView::OnLvnItemChanged(NMHDR* pNMHDR, LRESULT* plResult)
{
	if (((CCustomMultiListView*)GetParent())->GetMarquee() != NULL) {
		if (((CCustomMultiListView*)GetParent())->GetMarquee() == GetSafeHwnd()) {
				*plResult = ((CCustomMultiListView*)GetParent())->
						ReflectNotifyMessage(GetSafeHwnd(), WM_NOTIFY, pNMHDR->idFrom, (LPARAM)pNMHDR);
		} else if (((CCustomMultiListView*)GetParent())->GetMarquee() == ((CCustomMultiListView*)GetParent())->GetCurLock()) {
			if (((NMLISTVIEW*)pNMHDR)->uChanged & LVIF_STATE) {
				HWND hOldCurLock = ((CCustomMultiListView*)GetParent())->GetCurLock();
				((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
				GetListCtrl().SetItemState(((NMLISTVIEW*)pNMHDR)->iItem, ((NMLISTVIEW*)pNMHDR)->uNewState, ((NMLISTVIEW*)pNMHDR)->uOldState | ((NMLISTVIEW*)pNMHDR)->uNewState);
				((CCustomMultiListView*)GetParent())->SetCurLock(hOldCurLock);
			}
		}
	}
}

afx_msg void CCustomListView::OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* plResult)
{
	if (((CCustomMultiListView*)GetParent())->GetMarquee() != NULL) {
		if (((CCustomMultiListView*)GetParent())->GetMarquee() == GetSafeHwnd()) {
			*plResult = ((CCustomMultiListView*)GetParent())->
					ReflectNotifyMessage(GetSafeHwnd(), WM_NOTIFY, pNMHDR->idFrom, (LPARAM)pNMHDR);
		} else if (((CCustomMultiListView*)GetParent())->GetMarquee() == ((CCustomMultiListView*)GetParent())->GetCurLock()) {
			CWnd* pWnd = CWnd::FromHandle(((CCustomMultiListView*)GetParent())->GetMarquee());
			//((LPNMLVSCROLL)pNMHDR)->dy not reliable as some notifications are dropped
			//in report view the delta is the number of lines not pixels contradicting the documentation
			if (GetScrollPos(SB_VERT) != (pWnd->GetScrollPos(SB_VERT) + ((LPNMLVSCROLL)pNMHDR)->dy)) {
				HWND hOldCurLock = ((CCustomMultiListView*)GetParent())->GetCurLock();
				((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
				CRect ItemRect;
				GetListCtrl().GetSubItemRect(0, 0, LVIR_BOUNDS, ItemRect);
				GetListCtrl().Scroll(CPoint(0, (ItemRect.bottom - ItemRect.top) * (pWnd->GetScrollPos(SB_VERT) + ((LPNMLVSCROLL)pNMHDR)->dy - GetScrollPos(SB_VERT))));
				((CCustomMultiListView*)GetParent())->SetCurLock(hOldCurLock);
			}
		}
	}
}

afx_msg void CCustomListView::OnLvnOdFindItem(NMHDR* pNMHDR, LRESULT* plResult)
{
	ASSERT((((NMLVFINDITEM*)pNMHDR)->lvfi.flags & LVFI_ALL) ==
			((NMLVFINDITEM*)pNMHDR)->lvfi.flags);
	*plResult = ((CCustomMultiListView*)GetParent())->FindItem(((NMLVFINDITEM*)pNMHDR)->lvfi.psz,
						((NMLVFINDITEM*)pNMHDR)->iStart,
						((NMLVFINDITEM*)pNMHDR)->lvfi.flags & (LVFI_PARTIAL | LVFI_SUBSTRING),
						((NMLVFINDITEM*)pNMHDR)->lvfi.flags & LVFI_WRAP);										
}

LRESULT CCustomListView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	//must suppress WM_TIMER for WM_RBUTTONDOWN which on the other listviews does a scroll wait and ensure visible
	//this will cause LVN_ODFINDITEM to be generated for each listview on WM_CHAR
	if (message >= WM_MOUSEFIRST && message <= WM_MBUTTONDBLCLK ||
		message >= WM_XBUTTONDOWN && message <= WM_XBUTTONDBLCLK ||
		message == WM_CHAR || message == WM_SYSCHAR ||
		message == WM_KEYDOWN || message == WM_KEYUP ||
		message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) {
		if (((CCustomMultiListView*)GetParent())->GetCurLock() == NULL) {
			((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
			LPARAM lParamNew = lParam;
			if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST) {
				//all mouse messages have coordinates which must translate the x coordinate
				UINT uFlags;
				int iItem = GetListCtrl().HitTest(CPoint(lParam), &uFlags);
				if (uFlags & LVHT_TORIGHT) {
					//must be to the right in the others as well
					//GetListCtrl().GetSubItemRect(iItem, GetListCtrl().GetHeaderCtrl()->GetItemCount() - 1, LVIR_BOUNDS, rect);
					//lParamNew = MAKELPARAM(LOWORD(lParam) - rect.right, HIGHWORD(lParam));
				} else if (uFlags & LVHT_NOWHERE) {
				} else if (	uFlags &
							(LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON)) {
					//either use 0 or must calculate seperately for each reflection...
					lParamNew = MAKELPARAM(0, HIWORD(lParam));
				}
			}
			((CCustomMultiListView*)GetParent())->
						ReflectMessage(GetSafeHwnd(), message, wParam, lParamNew);
			lResult = CListView::WindowProc(message, wParam, lParam);
			((CCustomMultiListView*)GetParent())->SetCurLock(NULL);
		} else if (((CCustomMultiListView*)GetParent())->GetCurLock() != GetSafeHwnd()) {
			//if potentially scrolling then must push and pop the lock
			HWND hOldCurLock = ((CCustomMultiListView*)GetParent())->GetCurLock();
			((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
			lResult = CListView::WindowProc(message, wParam, lParam);
			((CCustomMultiListView*)GetParent())->SetCurLock(hOldCurLock);
		} else lResult = 0;
	} else if (message == WM_MOUSEWHEEL) {
		((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
		lResult = CListView::WindowProc(message, wParam, lParam);
		((CCustomMultiListView*)GetParent())->
					ReflectMessage(GetSafeHwnd(), WM_VSCROLL,
							MAKELONG(SB_THUMBTRACK, GetScrollPos(SB_VERT)),
							(LPARAM)GetScrollBarCtrl(SB_VERT)->GetSafeHwnd());
		((CCustomMultiListView*)GetParent())->SetCurLock(NULL);
	} else lResult = CListView::WindowProc(message, wParam, lParam);
	return lResult;
}

afx_msg void CCustomListView::OnVScroll(UINT nSBCode, UINT nPos,
										CScrollBar *pScrollBar)
{
	if (((CCustomMultiListView*)GetParent())->GetCurLock() == NULL) {
		((CCustomMultiListView*)GetParent())->SetCurLock(GetSafeHwnd());
		CListView::OnVScroll(nSBCode, nPos, pScrollBar);
		((CCustomMultiListView*)GetParent())->
								ReflectMessage(	GetSafeHwnd(), WM_VSCROLL, MAKELONG(nSBCode, nPos),
												(LPARAM)pScrollBar->GetSafeHwnd());
		((CCustomMultiListView*)GetParent())->SetCurLock(NULL);
	} else if (((CCustomMultiListView*)GetParent())->GetCurLock() != GetSafeHwnd()) {
		CListView::OnVScroll(nSBCode, nPos, pScrollBar);
		if (nSBCode == SB_THUMBTRACK) {
			CRect ItemRect;
			GetListCtrl().GetSubItemRect(0, 0, LVIR_BOUNDS, ItemRect);
			GetListCtrl().Scroll(CPoint(0, (ItemRect.bottom - ItemRect.top) * ((WORD)nPos - GetScrollPos(SB_VERT))));
		}
	}
}

afx_msg void CCustomListView::OnLvnClickCustomList(NMHDR *pNMHDR,
												   LRESULT * /*pResult*/)
{
	DWORD dwDepth;
	void* Entry;
	CRect rect;
	if (((LPNMITEMACTIVATE)pNMHDR)->iItem != -1 &&
		GetColumnMap()->GetTreeColIndex() != -1 &&
		GetCurrentModel()->HasChild(GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(((LPNMITEMACTIVATE)pNMHDR)->iItem)))) {
		dwDepth = GetCurrentModel()->GetDepth(GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(((LPNMITEMACTIVATE)pNMHDR)->iItem)));
		GetListCtrl().GetItemRect((int)((LPNMITEMACTIVATE)pNMHDR)->iItem, rect, LVIR_LABEL);
		rect.left += 2 - GetSystemMetrics(SM_CXSMICON) + dwDepth * 16;
		rect.right = rect.left + 8;
		rect.top += (rect.bottom - rect.top - 8) >> 1;
		rect.bottom = rect.top + 8;
		if ((Entry = GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(((LPNMITEMACTIVATE)pNMHDR)->iItem))) != NULL) {
			if ((((LPNMITEMACTIVATE)pNMHDR)->ptAction.x >= rect.left) &&
				(((LPNMITEMACTIVATE)pNMHDR)->ptAction.x <= rect.right)) {
				if ((((LPNMITEMACTIVATE)pNMHDR)->ptAction.y >= rect.top) &&
					(((LPNMITEMACTIVATE)pNMHDR)->ptAction.y <= rect.bottom)) {
					WORD wExpand;
					if (!GetExpandedMap()->Lookup(Entry, wExpand)) wExpand = FALSE;
					wExpand = (wExpand ? FALSE : TRUE);
					GetExpandedMap()->SetAt(Entry, wExpand);
					((CCustomMultiListView*)GetParent())->ExpandCollapseTree(Entry);
					if (wExpand) {
						//sort...
					}
					//doubt this is necessary?
					/*RedrawItems(((LPNMITEMACTIVATE)pNMHDR)->iItem,
											((LPNMITEMACTIVATE)pNMHDR)->iItem);*/
				}
			}
		}
	}
}


afx_msg void CCustomListView::OnLvnRClickCustomList(NMHDR *pNMHDR,
													LRESULT * /*pResult*/)
{
	int iIndex;
	CStringArray CategoryArray;
	CArray<CMenu*> MenuArray;
	DWORD dwCount;
	DisplaySet* pSet = GetDisplaySet();
	CMenu Menu;
	CRect rect;
	CString Str;
	POINT Point;
	INT_PTR iCategory;
	BOOL bOnlyDel = FALSE;
	if (((LPNMITEMACTIVATE)pNMHDR)->iItem == -1) {
		Menu.CreatePopupMenu();
		GetWindowRect(rect);
		for (	dwCount = 0;
				dwCount < GetColumnMap()->GetColumnCount(); dwCount++) {
			GetCurrentModel()->Render(&pSet->paDisplayColumns[dwCount].pCaptionRendererInfo, NULL, &Str);
			Menu.AppendMenu(MF_STRING |
							(GetColumnMap()->GetColumnSelected(dwCount) ?
								MF_CHECKED : 0),
							dwCount + 1,
							Str);
		}
		GetCursorPos(&Point);
		if ((dwCount = Menu.TrackPopupMenu(
							TPM_RETURNCMD | TPM_LEFTALIGN |
							TPM_RIGHTBUTTON, Point.x, Point.y,
							this, NULL)) != 0) {
			if (GetColumnMap()->GetColumnSelected(dwCount - 1)) {
				bOnlyDel = (GetColumnMap()->GetColumnGroup(dwCount - 1) ==
							(DWORD)GetColGroupIndex());
				//must delete through parent....
				//
				if (bOnlyDel) {
					GetListCtrl().DeleteColumn(GetColumnMap()->GetColumnIndex(dwCount - 1)); //trace error log
				} else {
					//parent must process since deleting from another
					((CCustomMultiListView*)GetParent())->GetListByColIndex(dwCount - 1)->GetListCtrl().DeleteColumn(GetColumnMap()->GetColumnIndex(dwCount - 1));
				}
				if (GetColGroupCount() != GetColGroupIndex() - 1) {
					AutoSetColumnWidth(GetColumnMap()->GetColumnFromIndex(GetColumnMap()->GetColumnIndex(dwCount - 1) - 1, (DWORD)GetColGroupIndex()));
				}
				GetColumnMap()->SetColumnSelected(dwCount - 1, FALSE);
			}
			if (!bOnlyDel && 
				!GetColumnMap()->GetColumnSelected(dwCount - 1)) {
				GetColumnMap()->SetColumnSelected(dwCount - 1, TRUE);
				GetColumnMap()->SetColumnGroup(dwCount - 1, (DWORD)GetColGroupIndex());
				//parent processes header column inserts
				((CCustomMultiListView*)GetParent())->GetListByColIndex(dwCount - 1)->InsertHeaderColumn(dwCount - 1);
			}
		}
		Menu.DestroyMenu();
	} else {
		Menu.CreatePopupMenu();
		for (dwCount = 0; pSet->paContextActionItems[dwCount].pCaptionRenderer.pRenderer; dwCount++) {
			for (	iCategory = CategoryArray.GetCount() - 1;
					iCategory != -1; iCategory--) {
				if (_tcscmp(CategoryArray[iCategory], 
							pSet->
											paContextActionItems[dwCount].
														szCategory) == 0) {
					break;
				}
			}
			//lookup category if one already exists
			if (iCategory == -1) {
				iCategory = CategoryArray.Add(pSet->
											paContextActionItems[dwCount].
														szCategory);
				CMenu* pMenu = new CMenu();
				if (pMenu) {
					if (pMenu->CreatePopupMenu())
						MenuArray.Add(pMenu);
					else
						delete pMenu;
				}
			}
			GetCurrentModel()->Render(&pSet->
								paContextActionItems[dwCount].pCaptionRenderer, (void*)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(((LPNMITEMACTIVATE)pNMHDR)->iItem)), &Str);
			MenuArray[iCategory]->AppendMenu(	MF_STRING, dwCount + 1,
												Str);
		}
		for (	iCategory = 0; iCategory < CategoryArray.GetCount();
				iCategory++) {
			Menu.AppendMenu(MF_POPUP, (UINT_PTR)(HMENU)*MenuArray[iCategory],
							CategoryArray[iCategory]);
		}
		Menu.AppendMenu(MF_STRING, 0x1000, _T("Copy Field"));
		Menu.AppendMenu(MF_STRING, 0x1001, _T("Copy"));
		GetWindowRect(rect);
		if ((iIndex = Menu.TrackPopupMenu(
					TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					rect.left + ((LPNMITEMACTIVATE)pNMHDR)->ptAction.x,
					rect.top + ((LPNMITEMACTIVATE)pNMHDR)->ptAction.y,
					this, NULL)) != 0) {
			if (iIndex == 0x1000 || iIndex == 0x1001) {
				CString String;
				DWORD dwSubCount;
				CArray<int, int&> Keys;
				//first process focus selection
				Keys.Add(((LPNMITEMACTIVATE)pNMHDR)->iItem);
				//after process multiple selection
				POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
				while (pos) {
					int iItem = GetListCtrl().GetNextSelectedItem(pos);
					if (iItem != ((LPNMITEMACTIVATE)pNMHDR)->iItem) {
						Keys.Add(iItem);
					}
				}
				if (iIndex == 0x1000) {
					LVHITTESTINFO lvhti = { ((LPNMITEMACTIVATE)pNMHDR)->ptAction };
					GetListCtrl().SubItemHitTest(&lvhti);
					for (dwSubCount = 0; dwSubCount < (DWORD)Keys.GetCount(); dwSubCount++) {
						GetSubItemString(lvhti.iSubItem, Keys[dwSubCount], Str);
						String += Str;
						if (dwSubCount != Keys.GetCount()) String += _T("\r\n");
					}
				} else if (iIndex == 0x1001) {
					for (dwSubCount = 0; dwSubCount < (DWORD)Keys.GetCount(); dwSubCount++) {
						for (dwCount = 0; dwCount < (DWORD)GetListCtrl().GetHeaderCtrl()->GetItemCount(); dwCount++) {
							GetSubItemString(dwCount, Keys[dwSubCount], Str);
							String += Str;
							if (dwCount != (DWORD)GetListCtrl().GetHeaderCtrl()->GetItemCount()) String += _T("\t");
						}
						if (dwSubCount != Keys.GetCount()) String += _T("\r\n");
					}
				}
				OpenClipboard();
				HANDLE hMem = GlobalAlloc(GMEM_MOVEABLE, (String.GetLength() + 1) * sizeof(TCHAR));
				memcpy(GlobalLock(hMem), String.GetString(), (String.GetLength() + 1) * sizeof(TCHAR));
				GlobalUnlock(hMem);
				SetClipboardData(sizeof(TCHAR) == sizeof(WCHAR) ? CF_UNICODETEXT : CF_TEXT, hMem);
				CloseClipboard();
			} else {
				CArray<void*, void*&> Keys;
				//first process focus selection
				Keys.Add((void*&)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(((LPNMITEMACTIVATE)pNMHDR)->iItem)));
				//after process multiple selection
				POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
				while (pos) {
					int iItem = GetListCtrl().GetNextSelectedItem(pos);
					if (iItem != ((LPNMITEMACTIVATE)pNMHDR)->iItem) {
						Keys.Add((void*&)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(iItem)));
					}
				}
				GetCurrentModel()->OnColumnAction(&pSet->paContextActionItems[iIndex - 1], &Keys);
			}
		}
		for (	iCategory = 0; iCategory < MenuArray.GetCount();
				iCategory++) {
			MenuArray[iCategory]->DestroyMenu();
			delete MenuArray[iCategory];
		}
		Menu.DestroyMenu();
	}
}


afx_msg void CCustomListView::OnLvnColumnClickCustomList(NMHDR *pNMHDR,
														 LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = (LPNMLISTVIEW)pNMHDR;
	DWORD dwOldSel = GetColumnMap()->GetColumnFromSortOrder(0);
	GetColumnMap()->SetColumnSortDirection(GetColumnMap()->GetColumnFromIndex(pNMLV->iSubItem, (DWORD)GetColGroupIndex()), 
										(GetColumnMap()->GetColumnSelected(dwOldSel) != GetColumnMap()->GetColumnSelected(GetColumnMap()->GetColumnFromIndex(pNMLV->iSubItem, (DWORD)GetColGroupIndex())) || pNMLV->iSubItem != (int)GetColumnMap()->GetColumnIndex(dwOldSel)) ?
										TRUE : !GetColumnMap()->GetColumnSortDirection(GetColumnMap()->GetColumnFromIndex(pNMLV->iSubItem, (DWORD)GetColGroupIndex())));
	DWORD dwOldSortIndex = GetColumnMap()->GetColumnSortOrder(pNMLV->iSubItem);
	GetColumnMap()->SetColumnSortOrder(GetColumnMap()->GetColumnFromIndex(pNMLV->iSubItem, (DWORD)GetColGroupIndex()), 0);
	//parents job since it may remove the indicator from another control
    ((CCustomMultiListView*)GetParent())->SetHeaderSortItem(dwOldSel);
	((CCustomMultiListView*)GetParent())->DoSort();
	*pResult = 0;
}

CMapPtrToWord* CCustomListView::GetExpandedMap()
{ return ((CCustomMultiListView*)GetParent())->GetExpandedMap(); }

CArray<void*>* CCustomListView::GetIndexArray()
{ return ((CCustomMultiListView*)GetParent())->GetIndexArray(); }

CArray<INT_PTR>* CCustomListView::GetSortIndexArray()
{ return ((CCustomMultiListView*)GetParent())->GetSortIndexArray(); }

CEnumModel* CCustomListView::GetCurrentModel()
{ return ((CCustomMultiListView*)GetParent())->GetCurrentModel(); }

INT_PTR CCustomListView::GetColGroupCount()
{ return ((CCustomMultiListView*)GetParent())->
									GetColumnMap()->GetColumnGroupCount(); }

INT_PTR CCustomListView::GetColGroupIndex()
{ return ((CCustomMultiListView*)GetParent())->GetColGroupIndex(&GetListCtrl()); }

CColumnMap* CCustomListView::GetColumnMap()
{ return ((CCustomMultiListView*)GetParent())->GetColumnMap(); }

DisplaySet* CCustomListView::GetDisplaySet()
{ return ((CCustomMultiListView*)GetParent())->GetDisplaySet();
}

////////////////////////////////////////////////////////////////////////////////
// CHexView
IMPLEMENT_DYNCREATE(CHexView, CSplitterWndEx)

BEGIN_MESSAGE_MAP(CHexView, CSplitterWndEx)
//{{AFX_MSG_MAP(CHewView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_MESSAGE(WM_PESTRID_UPDATEUI, OnUpdateUI)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CCustomMultiListView
IMPLEMENT_DYNCREATE(CCustomMultiListView, CSplitterWndEx)

BEGIN_MESSAGE_MAP(CCustomMultiListView, CSplitterWndEx)
//{{AFX_MSG_MAP(CCustomMultiListView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_PESTRID_UPDATEUI, OnUpdateUI)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()