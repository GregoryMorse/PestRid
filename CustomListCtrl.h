#pragma once
#include "CustomSplit.h"
#include "Modules.h"

#define MAX_COLGROUPS	2 //change limit to 32 by enhancing CSplitterWndEx

////////////////////////////////////////////////////////////////////////////////
// CColumnMap

//fill in column width aspect too...
class CColumnMap
{
public:
	CColumnMap(DisplaySet* pDisplaySet)
	{
		m_dwTreeColIndex = ~0UL;
		m_dwColumnCount = 0;
		while (pDisplaySet->paDisplayColumns[m_dwColumnCount].pRendererInfo.pRenderer) {
			m_dwColumnCount++;
		}
		m_ColumnMap.SetSize(m_dwColumnCount);
		ReadColumnMap(pDisplaySet);
	}
	DWORD GetColumnCount() { return m_dwColumnCount; }
	DWORD GetColumnIndex(DWORD dwColumn)
	{
		DWORD dwCount;
		DWORD dwColCount = 0;
		if (!m_ColumnMap[dwColumn].bSelected) return ~0UL;
		for (dwCount = 0; dwCount < dwColumn; dwCount++) {
			if (m_ColumnMap[dwColumn].dwColGroup == 
				m_ColumnMap[dwCount].dwColGroup) dwColCount++;
		}
		return dwColCount;
	}
	DWORD GetColumnFromIndex(DWORD dwIndex, DWORD dwColGroup)
	{
		DWORD dwCount;
		DWORD dwColCount = 0;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].dwColGroup == dwColGroup) {
				if (dwColCount == dwIndex) return dwCount;
				dwColCount++;
			}
		}
		return ~0UL;
	}
	DWORD GetColumnFromSortOrder(DWORD dwSortOrder)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].dwSortOrder == dwSortOrder) {
				return dwCount;
			}
		}
		return ~0UL;
	}
	DWORD GetColumnTypeCount(DWORD dwColGroup, BOOL bSelected)
	{
		DWORD dwCount;
		DWORD dwColCount = 0;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].bSelected == bSelected &&
				m_ColumnMap[dwCount].dwColGroup == dwColGroup) dwColCount++;
		}
		return dwColCount;
	}
	DWORD GetColumnGroupCount()
	{ 
		DWORD dwCount;
		DWORD dwColGroup = 0;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].dwColGroup > dwColGroup) {
				dwColGroup = m_ColumnMap[dwCount].dwColGroup;
			}
		}
		return m_dwColumnCount ? dwColGroup + 1 : 0;
	}
	DWORD GetTreeColIndex() { return m_dwTreeColIndex; }
	DWORD GetColumnGroup(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].dwColGroup; }
	BOOL GetColumnSelected(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].bSelected; }
	DWORD GetColumnOrder(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].dwOrder; }
	DWORD GetColumnSortOrder(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].dwSortOrder; }
	BOOL GetColumnSortDirection(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].bSortAscending; }
	DWORD GetColumnWidth(DWORD dwColumn)
	{ return m_ColumnMap[dwColumn].dwWidth; }
	//Must fix sort order
	void SetTreeColIndex(DWORD dwColumn) { m_dwTreeColIndex = dwColumn; }
	void SetColumnGroup(DWORD dwColumn, DWORD dwColGroup)
	{ m_ColumnMap[dwColumn].dwColGroup = dwColGroup; }
	void SetColumnSelected(DWORD dwColumn, BOOL bSelected)
	{ m_ColumnMap[dwColumn].bSelected = bSelected; }
	void SetColumnOrder(DWORD dwColumn, DWORD dwOrder)
	{ m_ColumnMap[dwColumn].dwOrder = dwOrder; }
	void SetColumnSortOrder(DWORD dwColumn, DWORD dwSortOrder)
	{
		DWORD dwCount;
		DWORD dwOldSortOrder = m_ColumnMap[dwColumn].dwSortOrder;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (dwCount == dwColumn) {
				m_ColumnMap[dwColumn].dwSortOrder = dwSortOrder;
			} else if (	m_ColumnMap[dwCount].dwSortOrder >= dwSortOrder &&
						m_ColumnMap[dwCount].dwSortOrder < dwOldSortOrder) {
				m_ColumnMap[dwCount].dwSortOrder++;
			}
		}
	}
	void SetColumnSortDirection(DWORD dwColumn, BOOL bSortAscending)
	{ m_ColumnMap[dwColumn].bSortAscending = bSortAscending; }
	void SetColumnWidth(DWORD dwColumn, DWORD dwWidth)
	{ m_ColumnMap[dwColumn].dwWidth = dwWidth; }
	void SetDefaults(DisplaySet* pDisplaySet)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			m_ColumnMap[dwCount].dwOrder = 
				pDisplaySet->paDisplayColumns[dwCount].dwDefaultOrder;
			m_ColumnMap[dwCount].dwSortOrder = 
				pDisplaySet->paDisplayColumns[dwCount].dwDefaultSortOrder;
			m_ColumnMap[dwCount].bSortAscending = 
				pDisplaySet->paDisplayColumns[dwCount].bDefaultSortAscending;
			m_ColumnMap[dwCount].bSelected = 
				pDisplaySet->paDisplayColumns[dwCount].bDefaultSelected;
			m_ColumnMap[dwCount].dwColGroup = 
				pDisplaySet->paDisplayColumns[dwCount].dwDefaultColGroup;
			m_ColumnMap[dwCount].dwWidth = 0; //calculate should define const
		}
	}
	static int __cdecl OrderCompare(const void* Col1, const void* Col2);
	static int __cdecl SortOrderCompare(const void* Col1, const void* Col2);
	BOOL VerifyColumnData()
	{
		DWORD dwCount;
		DWORD dwSubCount;
		DWORD dwCurColGroup;
		DWORD dwCurOrder;
		CArray<ColumnMap*> SortArray;
		SortArray.SetSize(m_dwColumnCount);
		//create pointer array for sorting
		dwSubCount = 0;
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].bSelected) {
				SortArray[dwSubCount++] = &m_ColumnMap[dwCount];
			}
		}
		qsort(	SortArray.GetData(), dwSubCount,
				sizeof(ColumnMap*), OrderCompare);
		dwCurColGroup = 0;
		dwCurOrder = 0;
		//for rebasing column group numbering to 0
		/*for (dwCount = 0; dwCount < dwSubCount; dwCount++) {
			if (SortArray[dwCount]->dwColGroup == dwCurColGroup) {
			} else if (SortArray[dwCount]->dwColGroup == dwCurColGroup + 1) {
				SortArray[dwCount]->dwColGroup =
							(dwCount == 0 ? dwCurColGroup : ++dwCurColGroup);
			} else {
			}
		}*/
		//resort based on sort order
		qsort(	SortArray.GetData(), dwSubCount,
				sizeof(ColumnMap*), SortOrderCompare);
		//check and fix
		for (dwCount = 0; dwCount < m_dwColumnCount; dwCount++) {
			if (m_ColumnMap[dwCount].bSortAscending &&
				m_ColumnMap[dwCount].bSortAscending != TRUE) {
				m_ColumnMap[dwCount].bSortAscending = TRUE;
			}
			if (m_ColumnMap[dwCount].bSelected &&
				m_ColumnMap[dwCount].bSelected != TRUE) {
				m_ColumnMap[dwCount].bSelected = TRUE;
			}
		}
		return TRUE;
	}
	void ReadColumnMap(DisplaySet* pDisplaySet)
	{
		CByteArray RegDataArray;
		//use default index here not pvArg
		if (g_PestRidSettings.ReadColumnMap(RegDataArray,
											(LPCTSTR)pDisplaySet->pTreeRenderer.pvArg)) {
			memcpy(	m_ColumnMap.GetData(), RegDataArray.GetData(),
					RegDataArray.GetSize());
			if (!VerifyColumnData()) SetDefaults(pDisplaySet);
		} else {
			SetDefaults(pDisplaySet);
		}
	}
	void WriteColumnMap(DisplaySet* pDisplaySet)
	{
		CByteArray RegDataArray;
		memcpy(	RegDataArray.GetData(), m_ColumnMap.GetData(),
				m_ColumnMap.GetSize() * sizeof(ColumnMap));
		//use default index here not pvArg
		if (g_PestRidSettings.WriteColumnMap(	RegDataArray,
												(LPCTSTR)pDisplaySet->pTreeRenderer.pvArg)) {
		}
	}
protected:
	//structure alignment must stay constant for compatibility
	struct ColumnMap
	{
		DWORD dwOrder;
		DWORD dwSortOrder;
		BOOL bSortAscending;
		DWORD dwWidth;
		BOOL bSelected:1;
		DWORD dwColGroup:31;
	};
	DWORD m_dwTreeColIndex;
	DWORD m_dwColumnCount;
	CArray<ColumnMap> m_ColumnMap;
};

////////////////////////////////////////////////////////////////////////////////
// CCustomHeaderCtrl

//Marlett font characters for Common Controls 6.0 sort up and down arrows
#define CHAR_SORTUP		_T("5")
#define CHAR_SORTDOWN	_T("6")

class CCustomHeaderCtrl : public CHeaderCtrl
{
public:
	int GetTotalColumnWidth()
	{
		int cx;
		DWORD dwColIndex;
		cx = 0;
		HDITEM hdi = { HDI_WIDTH };
		for (	dwColIndex = GetItemCount() - 1; dwColIndex != -1; dwColIndex--) {
			GetItem(dwColIndex, &hdi);
			cx += hdi.cxy;
		}
		return cx;
	}
	int GetIdealTotalColumnWidth()
	{
		int cx;
		DWORD dwColIndex;
		cx = 0;
		for (	dwColIndex = GetItemCount() - 1; dwColIndex != -1; dwColIndex--) {
			cx += GetColumnMap()->GetColumnWidth(dwColIndex);
		}
		return cx;
	}
	int CalculateHeaderWidth(DWORD dwIndex)
	{
		//does not support themes
		SIZE textSize;
		int cxBitmapWidth;
		CDC* pDC;
		CGdiObject* pOldFont;
		CString Str;
		BOOL bSorted = GetColumnMap()->GetColumnFromSortOrder(0) == dwIndex;
		BOOL bAscending = GetColumnMap()->GetColumnSortDirection(dwIndex);
		GetCurrentModel()->Render(&GetDisplaySet()->
					paDisplayColumns[dwIndex].pCaptionRendererInfo, NULL, &Str);
		pDC = GetDC();
		cxBitmapWidth = bSorted ? 
						CalculateSortArrowWidth(pDC, bAscending) : 0;
		if (cxBitmapWidth == -1) return -1;
		pOldFont = pDC->SelectObject(GetFont());
		if (!GetTextExtentPoint(pDC->GetSafeHdc(), Str, 
								(int)Str.GetLength(), &textSize)) {
			return -1;
		}
		pDC->SelectObject(pOldFont);
		ReleaseDC(pDC);
		return	GetSystemMetrics(SM_CXEDGE) * 3 * 2 + //text margins
				textSize.cx + //text size
				//bitmap margins
				GetBitmapMargin() * 2 +
				cxBitmapWidth; //bitmap size
	}
protected:
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
	{
		HDHITTESTINFO hti;
		if (nHitTest == HTCLIENT) {
			hti.iItem = 0;
			hti.flags = 0;
			//for possible efficiency could override PreTranslateMessage
			//  to get point out of MSG structure
			if (!GetCursorPos(&hti.pt)) ASSERT(FALSE);
			ScreenToClient(&hti.pt);
			if (SendMessage(HDM_HITTEST, 0, (LPARAM)&hti) != -1) {
				//prevent splitter cursor for last column
				if (hti.flags == HHT_ONDIVIDER &&
					hti.iItem == GetItemCount() - 1) {
					HCURSOR hCursor = LoadCursor(	AfxGetInstanceHandle(),
													IDC_ARROW);
					//must return before sending cursor msg so post
					//  or cursor will often misdraw
					if (GetCursor() != hCursor) {
						PostMessage(WM_SETCURSOR, (WPARAM)pWnd->GetSafeHwnd(),
									MAKELPARAM(nHitTest, message));
					}
					return TRUE;
				}
			}
		}
		return CHeaderCtrl::OnSetCursor(pWnd, nHitTest, message);
	}
	afx_msg void OnBeginTrack(NMHDR *pNMHDR, LRESULT *pResult)
	{
		//need way to calculate last column widths...
		//prevent tracking for last column
		//*pResult = (((NMHEADER*)pNMHDR)->iItem == GetItemCount() - 1);
	}
	afx_msg void OnHdnDividerDblClick(NMHDR* pNMHDR, LRESULT *pResult);
	afx_msg void OnHdnCustomDrawCustomHeader(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)pNMHDR;
		*pResult = CDRF_DODEFAULT;
		if (lpNMCustomDraw->dwDrawStage == CDDS_PREPAINT) {
			*pResult = CDRF_NOTIFYITEMDRAW;
		} else if (lpNMCustomDraw->dwDrawStage == CDDS_ITEMPREPAINT) {
			*pResult = CDRF_NOTIFYPOSTPAINT;
		} else if (	lpNMCustomDraw->dwDrawStage == CDDS_ITEMPOSTPAINT) {
			TCHAR szBuf[32];
			HFONT hFont = (HFONT)GetCurrentObject(lpNMCustomDraw->hdc, OBJ_FONT);
			LOGFONT lf;
			GetObject(hFont, sizeof(LOGFONT), &lf);
			lf.lfWidth = lf.lfWidth / 2;
			lf.lfHeight = lf.lfHeight * 2 / 3;
			_stprintf(szBuf, _T("%lu"), GetColumnMap()->GetColumnSortOrder(GetColumnMap()->GetColumnFromIndex((DWORD)lpNMCustomDraw->dwItemSpec, (DWORD)GetColGroupIndex())));
			CFont NewFont;
			NewFont.CreateFontIndirect(&lf);
			SelectObject(lpNMCustomDraw->hdc, NewFont);
			int iOldMode = SetBkMode(lpNMCustomDraw->hdc, TRANSPARENT);
			TextOut(lpNMCustomDraw->hdc, lpNMCustomDraw->rc.left + GetSystemMetrics(SM_CXBORDER), lpNMCustomDraw->rc.top, szBuf, (int)_tcslen(szBuf));
			SelectObject(lpNMCustomDraw->hdc, hFont);
			SetBkMode(lpNMCustomDraw->hdc, iOldMode);
			*pResult = CDRF_NOTIFYITEMDRAW;
		}
	}
	afx_msg void OnHdnGetDispInfoCustomHeader(NMHDR *pNMHDR, LRESULT *pResult)
	{
		CString Str;
		NMHDDISPINFO* pDispInfo = (NMHDDISPINFO*)pNMHDR;
		if (pDispInfo->mask & HDI_TEXT) {
			GetCurrentModel()->Render(&GetDisplaySet()->
										paDisplayColumns[GetColumnMap()->GetColumnFromIndex((DWORD)pDispInfo->lParam, (DWORD)GetColGroupIndex())].pCaptionRendererInfo, NULL, &Str);
			//allowing more than the default 264 bytes requires memory manager
			_tcsncpy(pDispInfo->pszText, Str, pDispInfo->cchTextMax);
			pDispInfo->pszText[pDispInfo->cchTextMax - 1] = 0;
		}
	}
	int CalculateSortArrowWidth(CDC* pHeaderDC, BOOL bAscending)
	{
		TEXTMETRIC tm;
		SIZE textSize;
		CFont pNewFont;
		CGdiObject* pOldFont;
		if (!PRU_CheckCommonControlsVersionMinimum(4, 7)) return -1;
		if (PRU_CheckCommonControlsVersionMinimum(6, 0)) {
			pHeaderDC->GetTextMetrics(&tm);			
			if (!pNewFont.CreateFont(	((	tm.tmHeight + tm.tmExternalLeading +
										GetSystemMetrics(SM_CYBORDER)) 
										& 0xFFFE) - 1,
									0, 0, 0, 0, 0, 0, 0,
									DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
									CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
									DEFAULT_PITCH, _T("Marlett"))) {
				return -1;
			}
			pOldFont = pHeaderDC->SelectObject(&pNewFont);
			if (!GetTextExtentPoint(pHeaderDC->GetSafeHdc(),
									bAscending ? CHAR_SORTUP : CHAR_SORTDOWN,
									1, &textSize)) {
				return -1;
			}
			pHeaderDC->SelectObject(pOldFont);
			pNewFont.DeleteObject();
			return textSize.cx + GetSystemMetrics(SM_CXEDGE) * 2;
		} else {
			return GetSystemMetrics(SM_CXICON);
		}
	}
	CColumnMap* GetColumnMap();
	CEnumModel* GetCurrentModel();
	INT_PTR GetColGroupIndex();
	DisplaySet* GetDisplaySet();
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////////
// CCustomListView

class CCustomListView : public CListView
{
public:
	CCustomListView() :	CListView() {}
	int Init();
	void Display()
	{
		int iCount;
		if (!IsWindowVisible()) {
			CHeaderCtrl* pHeaderCtrl = GetListCtrl().GetHeaderCtrl();
			ShowWindow(SW_SHOW);
			//must check if image column present
			//create main image list and a dummy image list
			if (!m_DummyImageList.GetSafeHandle()) {
				m_DummyImageList.Create(1, 16, ILC_MASK, 1, 0);
				BYTE* AndBuffer = new BYTE[	GetSystemMetrics(SM_CXSMICON) *
											GetSystemMetrics(SM_CYSMICON) / 8];
				BYTE* XorBuffer = new BYTE[	GetSystemMetrics(SM_CXSMICON) *
											GetSystemMetrics(SM_CYSMICON) / 8];
				HICON hDummyIcon;
				if (AndBuffer && XorBuffer) {
					memset(AndBuffer, 0,	GetSystemMetrics(SM_CXSMICON) * 
											GetSystemMetrics(SM_CYSMICON) / 8);
					memset(XorBuffer, 0xFF, GetSystemMetrics(SM_CXSMICON) *
											GetSystemMetrics(SM_CYSMICON) / 8);
					m_DummyImageList.Add(	hDummyIcon = 
								CreateIcon(	AfxGetApp()->m_hInstance,
											GetSystemMetrics(SM_CXSMICON),
											GetSystemMetrics(SM_CYSMICON),
											1, 1, AndBuffer, XorBuffer));
					if (hDummyIcon) DestroyIcon(hDummyIcon);
				}
				if (AndBuffer) delete [] AndBuffer;
				if (XorBuffer) delete [] XorBuffer;
			}
			m_IconIndexes.RemoveAll();
			if (m_ImageList.GetSafeHandle()) {
				//only way to clear out cxSmIcon space reserved for icons
				//  without recreating the listview control
				ImageList_SetIconSize(m_ImageList.GetSafeHandle(), 0, 0);
				GetListCtrl().SetImageList(&m_ImageList, LVSIL_SMALL);
				GetListCtrl().SetImageList(&m_ImageList, LVSIL_NORMAL);
				m_ImageList.DeleteImageList();
			}
			if (GetDisplaySet()->paDisplayColumns[0].pIconRenderer.pRenderer) {
				m_ImageList.Create(16, 16, ILC_COLOR32, 0, 16);
				GetListCtrl().SetImageList(&m_ImageList, LVSIL_SMALL);
				GetListCtrl().SetImageList(&m_ImageList, LVSIL_NORMAL);
			} else {
				GetListCtrl().SetImageList(NULL, LVSIL_NORMAL);
				pHeaderCtrl->SetImageList(NULL, LVSIL_NORMAL);
				GetListCtrl().SetImageList(NULL, LVSIL_SMALL);
				pHeaderCtrl->SetImageList(NULL, LVSIL_SMALL);
			}
			//cannot subclass until column insertion since header control
				//  is not created until columns inserted and control visible
			if (!m_HeaderCtrl.GetSafeHwnd()) {
				ASSERT(	pHeaderCtrl->GetSafeHwnd());
				if (!m_HeaderCtrl.SubclassWindow(
								pHeaderCtrl->GetSafeHwnd())) ASSERT(FALSE);
			}
			for (	iCount = 0;
					iCount < pHeaderCtrl->GetItemCount();
					iCount++) {
				AutoSetColumnWidth(iCount);
			}
		}
	}
	void AutoSetColumnWidth(DWORD dwIndex)
	{
		int cx;
		CRect Rect;
		DWORD dwColIndex = GetColumnMap()->GetColumnIndex(dwIndex);
		cx = ((CCustomHeaderCtrl*)GetListCtrl().GetHeaderCtrl())->CalculateHeaderWidth(dwIndex);
		if (GetListCtrl().GetItemCount() > 0) {
			if (GetListCtrl().GetColumnWidth(dwColIndex) < cx) {
				GetListCtrl().SetColumnWidth(dwColIndex, cx);
			} else {
				cx = GetListCtrl().GetColumnWidth(dwColIndex);
			}
		}
		for (int iIndex = 0; iIndex < min(GetListCtrl().GetItemCount(), GetListCtrl().GetCountPerPage()); iIndex++) {
			cx = max(cx, CalculateWidth(iIndex + GetListCtrl().GetTopIndex(), dwColIndex));
		}
		GetColumnMap()->SetColumnWidth(dwIndex, cx);
		GetListCtrl().SetColumnWidth(dwColIndex,
								GetColumnMap()->GetColumnWidth(dwIndex));
	}
	CColumnMap* GetColumnMap();
	CEnumModel* GetCurrentModel();
	INT_PTR GetColGroupIndex();
	DisplaySet* GetDisplaySet();
protected:
	BOOL PreCreateWindow(CREATESTRUCT & cs)
	{
		cs.style =	LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA |
					LVS_SHAREIMAGELISTS | LVS_ALIGNLEFT |
					WS_TABSTOP | WS_CHILD;// | LVS_NOSCROLL; //start hidden
		return CListView::PreCreateWindow(cs);
	}
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		if (CListView::OnCreate(lpCreateStruct) == -1) return -1;
		GetListCtrl().SetExtendedStyle(	LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT |
										LVS_EX_HEADERDRAGDROP);
		GetListCtrl().SendMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS));
		/*GetListCtrl().SendMessage(CCM_SETUNICODEFORMAT, 
#ifdef _UNICODE
			TRUE
#else
			FALSE
#endif
			, 0);*/
		SetFont(GetParent()->GetFont(), FALSE);
		return 0;
	}
	afx_msg void OnDestroy()
	{
		if (m_HeaderCtrl.GetSafeHwnd()) m_HeaderCtrl.UnsubclassWindow();
		if (m_DummyImageList.GetSafeHandle()) {
			if (!m_DummyImageList.DeleteImageList()) {
			}
		}
		if (m_ImageList.GetSafeHandle()) {
			if (!m_ImageList.DeleteImageList()) {
			}
		}
	}
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg void OnLvnOdFindItem(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy)
	{
		DWORD dwColIndex;
		//CRect /Rect;
		//fix last column width in non-last column group
		//GetClientRect(Rect);
		if (GetColumnMap()) {
			CHeaderCtrl* pHeaderCtrl = GetListCtrl().GetHeaderCtrl();
			if (GetColGroupCount() - 1 == GetColGroupIndex()) {
			//no action for last column as it is completely under user control
			} else if (pHeaderCtrl->GetItemCount() == 1) {
				GetListCtrl().SetColumnWidth(0, cx);
			} else {
				int iColWidth = ((CCustomHeaderCtrl*)pHeaderCtrl)->GetTotalColumnWidth();
				for (	dwColIndex =
						pHeaderCtrl->GetItemCount() - 1;
						dwColIndex != -1;
						dwColIndex--) {
					if (iColWidth > cx) {
						int iOldColWidth = GetListCtrl().GetColumnWidth(dwColIndex);
						GetListCtrl().SetColumnWidth(dwColIndex, max(0, iOldColWidth - (iColWidth - cx)));
						iColWidth -= (iOldColWidth - (max(0, iOldColWidth - (iColWidth - cx))));
					} else if (iColWidth < cx) {
						int iOldColWidth = GetListCtrl().GetColumnWidth(dwColIndex);
						GetListCtrl().SetColumnWidth(dwColIndex, min((int)GetColumnMap()->GetColumnWidth(dwColIndex), iOldColWidth + cx - iColWidth));
						iColWidth += min((int)GetColumnMap()->GetColumnWidth(dwColIndex), iOldColWidth + cx - iColWidth) - iOldColWidth;
					}
				}
				if (iColWidth != cx)
					GetListCtrl().SetColumnWidth(pHeaderCtrl->GetItemCount() - 1, 
											 max(0, GetListCtrl().GetColumnWidth(pHeaderCtrl->GetItemCount() - 1) + cx - iColWidth));
			}
		}
		CListView::OnSize(nType, cx, cy);
	}
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS *lpncsp)
	{
		if (GetColumnMap() && GetColGroupCount() - 1 != GetColGroupIndex()) {
			if (GetStyle() & WS_VSCROLL) {
				ModifyStyle(WS_VSCROLL, 0, 0);
			} 
			if (GetStyle() & WS_HSCROLL) {
				SetScrollRange(SB_HORZ, 0, 0, FALSE);
			}
		}
		CListView::OnNcCalcSize(bCalcValidRects, lpncsp);
	}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
		NMITEMACTIVATE nmiact;
		CHeaderCtrl* pHeaderCtrl = GetListCtrl().GetHeaderCtrl();
		//header control ID is typically 0 but handle manually in case
		if (pHeaderCtrl &&
			(int)wParam == pHeaderCtrl->GetDlgCtrlID()) {
			if (((NMHDR*)lParam)->code == NM_RCLICK) {
				//display menu
				nmiact.hdr.code = ((NMHDR*)lParam)->code;
				nmiact.hdr.hwndFrom = GetSafeHwnd();
				nmiact.hdr.idFrom = GetDlgCtrlID();
				nmiact.iItem = nmiact.iSubItem = -1;
				GetParent()->SendMessage(	WM_NOTIFY, nmiact.hdr.idFrom, 
											(LPARAM)&nmiact);
			} else if ((((NMHDR*)lParam)->code == HDN_ENDDRAG)) {
				int iOrder = -1;
				//update column map with new order
				if (PRU_CheckCommonControlsVersionMinimum(4, 7) &&
					(((NMHEADER*)lParam)->pitem->mask & HDI_ORDER)) {
					//need to check mask
					iOrder = ((NMHEADER*)lParam)->pitem->iOrder;
				} else {
					HDHITTESTINFO hti;
					hti.flags = 0;
					hti.iItem = 0;
                    if (!GetCursorPos(&hti.pt)) ASSERT(FALSE);
					ScreenToClient(&hti.pt);
					if (pHeaderCtrl->
							SendMessage(HDM_HITTEST, 0, (LPARAM)&hti) != -1) {
						if (hti.flags == HHT_ONDIVIDER) iOrder = hti.iItem;
					}
					//DeleteColumn(((NMHEADER*)lParam)->iItem);
					//InsertHeaderColumn(	pHeaderDC, dwCount, GetDisplaySet(), 
					//						GetColumnMap()->GetColumnTypeCount(
					//		GetColumnMap()->GetColumnSelected(dwCount)) == 1);
					//must delete column and reinsert it but where???
					//  how to for win95? just return TRUE in XP to figure out
				}
				GetColumnMap()->SetColumnOrder(
					GetColumnMap()->GetColumnFromIndex(
													((NMHEADER*)lParam)->iItem,
													(DWORD)GetColGroupIndex()),
					iOrder);
			}
		}
		return CListView::OnNotify(wParam, lParam, pResult);
	}
	afx_msg void OnLvnGetDispInfoCustomList(NMHDR *pNMHDR, LRESULT *pResult)
	{
		CString Str;
		NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
		if (pDispInfo->item.mask & LVIF_TEXT) {
			GetSubItemString(	pDispInfo->item.iSubItem,
									pDispInfo->item.iItem, Str);
			//allowing more than the default 264 bytes requires memory manager
			_tcsncpy(pDispInfo->item.pszText, Str, pDispInfo->item.cchTextMax);
			pDispInfo->item.pszText[pDispInfo->item.cchTextMax - 1] = 0;
		}
		if (pDispInfo->item.mask & LVIF_IMAGE && GetDisplaySet()->paDisplayColumns[0].pIconRenderer.pRenderer != NULL) {
			HICON hIcon = NULL;
			GetCurrentModel()->Render(&GetDisplaySet()->paDisplayColumns[0].pIconRenderer, (void*)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(pDispInfo->item.iItem)), &hIcon);
			//&GetDisplaySet()->
			//	paDisplayColumns[GetColumnMap()->GetColumnFromIndex(pDispInfo->item.iSubItem,
			//											(DWORD)GetColGroupIndex())].pIconRenderer
			while (m_IconIndexes.GetCount() < GetListCtrl().GetItemCount()) { m_IconIndexes.Add(~0UL); }
			if (m_IconIndexes[pDispInfo->item.iItem] != -1) {
				m_ImageList.Replace(pDispInfo->item.iImage = m_IconIndexes[pDispInfo->item.iItem], hIcon);
			} else {
				m_IconIndexes.SetAt(pDispInfo->item.iItem, pDispInfo->item.iImage = m_ImageList.Add(hIcon));
			}
			DestroyIcon(hIcon);
		}
		*pResult = 0;
	}
	afx_msg void OnLvnMarqueeBegin(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnLvnReleaseCapture(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnLvnItemChanged(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnLvnEndScroll(NMHDR* pNMHDR, LRESULT* plResult);
	afx_msg void OnLvnColumnClickCustomList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnClickCustomList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnRClickCustomList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnCustomDrawCustomList(NMHDR *pNMHDR, LRESULT *pResult)
	{
		LPNMLVCUSTOMDRAW lpNMCustomDraw = (LPNMLVCUSTOMDRAW)pNMHDR;
		*pResult = CDRF_DODEFAULT;
		if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_PREPAINT) {
			//if (GetColumnMap()->GetTreeColIndex() != -1) {
				*pResult = CDRF_NOTIFYITEMDRAW;
			//}
		} else if (lpNMCustomDraw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
			//highlight when not focused
			if (GetListCtrl().GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec,
										 LVIS_SELECTED) != 0) {
				lpNMCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
				lpNMCustomDraw->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
				lpNMCustomDraw->nmcd.uItemState &= ~CDIS_SELECTED;
				lpNMCustomDraw->nmcd.uItemState |= CDIS_FOCUS;
			}
			//if (GetColumnMap()->GetTreeColIndex() != -1) {
				if (GetColumnMap()->GetColumnFromIndex(
											(DWORD)lpNMCustomDraw->iSubItem,
											(DWORD)GetColGroupIndex()) ==
					GetColumnMap()->GetTreeColIndex()) {
					OnCustomDrawTreeList(lpNMCustomDraw);
					*pResult = CDRF_SKIPDEFAULT | CDRF_NOTIFYSUBITEMDRAW;
				} else {
					*pResult = CDRF_NOTIFYSUBITEMDRAW;
				}
			//} else {
			//	*pResult = CDRF_DODEFAULT;
			//}
		} else if (	lpNMCustomDraw->nmcd.dwDrawStage ==
					(CDDS_SUBITEM | CDDS_ITEMPREPAINT)) {
			lpNMCustomDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
			GetCurrentModel()->Color(&GetDisplaySet()->paDisplayColumns[GetColumnMap()->GetColumnFromIndex(lpNMCustomDraw->iSubItem,
													(DWORD)GetColGroupIndex())].pRendererInfo, (void*)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(lpNMCustomDraw->nmcd.dwItemSpec)), lpNMCustomDraw->clrText);
			if (GetListCtrl().GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec,
										 LVIS_SELECTED) != 0) {
				lpNMCustomDraw->clrText ^= GetSysColor(COLOR_HIGHLIGHTTEXT);
			}
			if (GetColumnMap()->GetColumnFromIndex(
											(DWORD)lpNMCustomDraw->iSubItem,
											(DWORD)GetColGroupIndex()) ==
				GetColumnMap()->GetTreeColIndex()) {
				OnCustomDrawTreeList(lpNMCustomDraw);
				*pResult = CDRF_SKIPDEFAULT | CDRF_NOTIFYSUBITEMDRAW;
			/*} else if (	GetColumnMap()->GetTreeColIndex() <
						lpNMCustomDraw->iSubItem) {
				*pResult = CDRF_NOTIFYSUBITEMDRAW; //CDRF_DODEFAULT?*/
			} else {
				*pResult = CDRF_NOTIFYSUBITEMDRAW;
			}
		}
	}
	void OnCustomDrawTreeList(LPNMLVCUSTOMDRAW lpNMCustomDraw)
	{
		void* Entry;
		CRect rect;
		CRect TreeButtonRect;
		CDC dc;
		DWORD dwCount;
		WORD wExpanded;
		COLORREF ref = 0;
		COLORREF bkref = 0;
		DWORD OldLeft;
		CBrush Brush;
		CPen Pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen* pOldObject;
		BOOL bSelected;
		CString Str;
		dc.Attach(lpNMCustomDraw->nmcd.hdc);
		dwCount = GetCurrentModel()->GetDepth(GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(lpNMCustomDraw->nmcd.dwItemSpec)));
		Entry = GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(lpNMCustomDraw->nmcd.dwItemSpec));
		GetListCtrl().GetItemRect((int)lpNMCustomDraw->nmcd.dwItemSpec,
					rect, LVIR_LABEL);
		OldLeft = rect.left;
		rect.left += 2 - GetSystemMetrics(SM_CXSMICON);
		//tree drawing and computation code
		rect.left += dwCount * 16;
		if (GetCurrentModel()->
						HasChild(GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(lpNMCustomDraw->nmcd.dwItemSpec)))) {
			TreeButtonRect = rect;
			TreeButtonRect.right = TreeButtonRect.left + 8;
			TreeButtonRect.top += (TreeButtonRect.bottom - TreeButtonRect.top - 8) >> 1;
			TreeButtonRect.bottom = TreeButtonRect.top + 8;
			pOldObject = dc.SelectObject(&Pen);
			dc.MoveTo(TreeButtonRect.left, TreeButtonRect.top);
			dc.LineTo(TreeButtonRect.right, TreeButtonRect.top);
			dc.LineTo(TreeButtonRect.right, TreeButtonRect.bottom);
			dc.LineTo(TreeButtonRect.left, TreeButtonRect.bottom);
			dc.LineTo(TreeButtonRect.left, TreeButtonRect.top);
			dc.MoveTo(TreeButtonRect.left + 2, ((TreeButtonRect.bottom - TreeButtonRect.top) >> 1) + TreeButtonRect.top);
			dc.LineTo(TreeButtonRect.right - 1, ((TreeButtonRect.bottom - TreeButtonRect.top) >> 1) + TreeButtonRect.top);
			if (GetExpandedMap()->Lookup(Entry, wExpanded) && wExpanded) {
				dc.MoveTo(((TreeButtonRect.right - TreeButtonRect.left) >> 1) + TreeButtonRect.left, TreeButtonRect.top + 2);
				dc.LineTo(((TreeButtonRect.right - TreeButtonRect.left) >> 1) + TreeButtonRect.left, TreeButtonRect.bottom - 1);
			}
			dc.SelectObject(pOldObject);
		}
		rect.left += 12;
		//code to draw tree/list
		HICON hIcon;
		GetCurrentModel()->Render(&GetDisplaySet()->
			paDisplayColumns[GetColumnMap()->GetColumnFromIndex(lpNMCustomDraw->iSubItem,
													(DWORD)GetColGroupIndex())].pIconRenderer, (void*)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(lpNMCustomDraw->nmcd.dwItemSpec)), &hIcon);
		if (hIcon) {
			if (!DrawIconEx(dc.GetSafeHdc(), rect.left, rect.top, hIcon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL)) {}
			DestroyIcon(hIcon);
		}
		rect.left += GetSystemMetrics(SM_CXSMICON);
		dc.SetTextAlign(TA_LEFT);					
		Brush.CreateSysColorBrush((GetListCtrl().GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED)) ? COLOR_HIGHLIGHT : COLOR_WINDOW);
		dc.FillRect(rect, &Brush);
		Brush.DeleteObject();
		if ((bSelected = 
			GetListCtrl().GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED)) != FALSE) {
			ref = dc.GetTextColor();
			bkref = dc.GetBkColor();
			dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
			dc.SetBkColor(GetSysColor(COLOR_HIGHLIGHT));
		}
		rect.left += 4;
		rect.top += 2;
		GetSubItemString(lpNMCustomDraw->iSubItem, (INT_PTR)lpNMCustomDraw->nmcd.dwItemSpec, Str);
		dc.DrawText(Str, rect, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
		rect.top -= 2;
		if (bSelected) {
			dc.SetTextColor(ref);
			dc.SetBkColor(bkref);
		}
		rect.left -= 4;
		if (GetListCtrl().GetItemState((int)lpNMCustomDraw->nmcd.dwItemSpec, LVIS_FOCUSED)) {
			dc.DrawFocusRect(rect);
		}
		//adjust item rect for drag and scroll stuff...
		dc.Detach();
		rect.left = OldLeft;
	}
	//recursive based on subitem
	void GetSubItemString(int iSubItem, INT_PTR iItem, CString& Str)
	{
		GetCurrentModel()->Render(&GetDisplaySet()->
			paDisplayColumns[GetColumnMap()->GetColumnFromIndex(iSubItem,
													(DWORD)GetColGroupIndex())].pRendererInfo, (void*)GetIndexArray()->GetAt(GetSortIndexArray()->GetAt(iItem)), &Str);
	}
	int CalculateWidth(int iItem, int iSubItem)
	{
		SIZE textSize;
		CString Str;
		CGdiObject* pOldFont;
		GetSubItemString(iSubItem, iItem, Str);
		CDC* pDC = GetDC();
		pOldFont = pDC->SelectObject(GetFont());
		if (!GetTextExtentPoint(pDC->GetSafeHdc(), Str, 
								(int)Str.GetLength(), &textSize)) {
			return -1;
		}
		pDC->SelectObject(pOldFont);
		ReleaseDC(pDC);
		return	GetSystemMetrics(SM_CXEDGE) * 3 * 2 + //text margins
				textSize.cx; //text size
	}
	void InsertHeaderColumn(DWORD dwIndex)
	{
		CString Str;
		DWORD dwColIndex = GetColumnMap()->GetColumnIndex(dwIndex);
		CHeaderCtrl* pHeaderCtrl = GetListCtrl().GetHeaderCtrl();
		//full width if the following TRUE:
		//GetColumnMap()->GetColumnTypeCount(GetColumnMap()->GetColumnGroup(dwIndex), TRUE) == 1
		LVCOLUMN column = { LVCF_WIDTH };
		column.cx = ((CCustomHeaderCtrl*)pHeaderCtrl)->CalculateHeaderWidth(dwIndex);
		dwColIndex = GetListCtrl().InsertColumn(dwColIndex, &column);
		HDITEM hdi = { HDI_LPARAM | HDI_TEXT | HDI_FORMAT }; //item index not reliable for notifications
		hdi.pszText = LPSTR_TEXTCALLBACK;
		hdi.cchTextMax = -1;
		hdi.fmt = HDF_STRING | HDF_LEFT;
		hdi.lParam = dwColIndex;
		pHeaderCtrl->SetItem(dwColIndex, &hdi);
		AutoSetColumnWidth(dwIndex);
	}
	CMapPtrToWord* GetExpandedMap();
	CArray<void*>* GetIndexArray();
	CArray<INT_PTR>* GetSortIndexArray();
	INT_PTR GetColGroupCount();

	CCustomHeaderCtrl m_HeaderCtrl;
	CImageList m_ImageList;
	CImageList m_DummyImageList;
	CDWordArray m_IconIndexes;

	DECLARE_DYNCREATE(CCustomListView)
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////////////////
// CHexView

class CHexView : public CSplitterWndEx
{
public:
	CHexView() :	CSplitterWndEx(TRUE),
								m_pCurrentModel(NULL),
								m_pSet(NULL) {}
	virtual ~CHexView()
	{ }
	BOOL Create(LPCTSTR /*lpszClassName*/, LPCTSTR /*lpszWindowName*/,
				DWORD dwStyle, const RECT& /*rect*/, CWnd* pParentWnd, UINT nID, 
				CCreateContext* pContext)
	{
		return CSplitterWnd::Create(pParentWnd, 1, 1, 
									CSize(10, 10), pContext,
									dwStyle | WS_CHILD | WS_VISIBLE, nID);
	}
	void OnSelChange(DisplaySet* pSet, CEnumModel* pModel, void* pvKey)
	{
		if (m_pCurrentModel) {
			m_pCurrentModel->Cleanup();
			delete m_pCurrentModel;
		}
		m_pSet = pSet;
		m_pCurrentModel = pSet ? new CEnumModel(pSet, FALSE, pModel, pvKey) : NULL;
	}
protected:
	int OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		int iRet = CSplitterWndEx::OnCreate(lpCreateStruct);
		SetFont(CFont::FromHandle((HFONT)GetStockObject(ANSI_FIXED_FONT)));
		return iRet;
	}
	afx_msg void OnDestroy()
	{
		CFrameWnd* pFrameWnd = EnsureParentFrame();	
		if (GetPane(0, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		DeleteView(0, 0);
		//delete windows before underlying data in case used before delete message received
		if (m_pCurrentModel) {
			m_pCurrentModel->Cleanup();
			delete m_pCurrentModel;
		}
		CSplitterWndEx::OnDestroy();
	}
	void PostNcDestroy()
	{
		// default for views is to allocate them on the heap
		//  the default post-cleanup is to 'delete this'.
		//  never explicitly call 'delete' on a view
		delete this;
	}
	afx_msg LRESULT OnUpdateUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		CArray<TreeViewChange*>* ViewChange;
		int iCount;
		//recurse through view change and update...
		if (m_pCurrentModel) {
			if ((ViewChange = m_pCurrentModel->GetViewChanges()) != NULL && ViewChange->GetCount()) {
				EndMenu();
				for (iCount = 0; iCount < ViewChange->GetCount(); iCount++) {
					//UpdateChange(ViewChange->GetAt(iCount));
				}
				//update size
				m_dwScrollPos = GetScrollPos(SB_VERT);
				m_pCurrentModel->Render(&m_pSet->paDisplayColumns[0].pRendererInfo, (void*)0, &m_ullOffset);
				m_pCurrentModel->Render(&m_pSet->paDisplayColumns[1].pRendererInfo, (void*)0, &m_ullLength);
				m_pCurrentModel->Render(&m_pSet->paDisplayColumns[2].pRendererInfo, (void*)0, &m_pBytes);
				TEXTMETRIC tm;
				CDC* pdc = GetDC();
				pdc->GetTextMetrics(&tm);
				ReleaseDC(pdc);
				//must scale height to maximum of INT_MAX
				SetWindowPos(NULL, 0, 0, tm.tmMaxCharWidth * (3 + GetAddressWidth() + 2 + m_dwColumnSize * 3 - 1 + 2 + m_dwColumnSize + 1),
							(int)((m_ullLength / m_dwColumnSize + ((m_ullLength % m_dwColumnSize) != 0 ? 1 : 0)) * (tm.tmHeight + tm.tmExternalLeading)),
							SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
				m_pCurrentModel->OnUpdateUI();
			}
		}
		return 0;
	}
	afx_msg void OnPaint()
	{
		DWORD dwCount;
		CPaintDC pdc(this);
		CString String;
		CString Format;
		Format.Format(_T(" %%0%lullX  "), GetAddressWidth());
		TEXTMETRIC tm;
		pdc.GetTextMetrics(&tm);
		for (dwCount = 0; dwCount < (m_pBytes->GetCount() / m_dwColumnSize); dwCount++) {
			String.Format(Format, m_ullOffset + (m_dwScrollPos + dwCount) * m_dwColumnSize);
			String += GetHexBytes(m_pBytes->GetData() + dwCount * m_dwColumnSize, m_dwColumnSize);
			String += _T("  ");
			String += GetAsciiBytes(m_pBytes->GetData() + dwCount * m_dwColumnSize, m_dwColumnSize);
			String += _T(" ");
			pdc.ExtTextOut(0, dwCount * (tm.tmHeight + tm.tmExternalLeading), 0, NULL, String, String.GetLength(), NULL);
		}
		if (m_pBytes->GetCount() % m_dwColumnSize) {
			String.Format(Format, m_ullOffset + dwCount * m_dwColumnSize);
			String += GetHexBytes(m_pBytes->GetData() + (m_dwScrollPos + dwCount) * m_dwColumnSize, m_pBytes->GetCount() % m_dwColumnSize);
			String += _T("  ");
			String += GetAsciiBytes(m_pBytes->GetData() + (m_dwScrollPos + dwCount) * m_dwColumnSize, m_pBytes->GetCount() % m_dwColumnSize);
			String += _T(" ");
			pdc.ExtTextOut(0, dwCount * (tm.tmHeight + tm.tmExternalLeading), 0, NULL, String, String.GetLength(), NULL);
		}
	}
private:
	DWORD GetAddressWidth()
	{
		DWORD dwCount;
		for (dwCount = 8; dwCount < 16; dwCount++) {
			if (((m_ullOffset + m_ullLength - 1) & (~0ULL & ~0ULL << (dwCount * 8))) == 0) break;
		}
		return dwCount;
	}
	DWORD m_dwColumnSize;
	DWORD m_dwScrollPos;
	unsigned long long m_ullOffset;
	unsigned long long m_ullLength;
	CByteArray* m_pBytes;
	DisplaySet* m_pSet;
	CEnumModel* m_pCurrentModel;

	DECLARE_DYNCREATE(CHexView)
	DECLARE_MESSAGE_MAP()
};

class CCustomMultiListView : public CSplitterWndEx
{
public:
	CCustomMultiListView() :	CSplitterWndEx(TRUE),
								m_pCurrentModel(NULL),
								m_pSet(NULL),
								m_ColumnMap(NULL), m_hCur(NULL) {}
	virtual ~CCustomMultiListView()
	{ }
	BOOL Create(LPCTSTR /*lpszClassName*/, LPCTSTR /*lpszWindowName*/,
				DWORD dwStyle, const RECT& /*rect*/, CWnd* pParentWnd, UINT nID, 
				CCreateContext* pContext)
	{
		BOOL bRet;
		CRuntimeClass* pOldViewClass = pContext->m_pNewViewClass;
		pContext->m_pNewViewClass = RUNTIME_CLASS(CCustomListView);
		bRet = CSplitterWnd::Create(pParentWnd, 1, MAX_COLGROUPS, 
									CSize(10, 10), pContext,
									dwStyle | WS_CHILD | WS_VISIBLE |
									SPLS_DYNAMIC_SPLIT, nID);
		pContext->m_pNewViewClass = pOldViewClass;
		return bRet;
	}
	void OnSelChange(DisplaySet* pSet, CEnumModel* pModel, void* pvKey)
	{
		int iCount;
		if (m_ColumnMap) {
			delete m_ColumnMap;
			m_ColumnMap = NULL;
		}
		if (m_pCurrentModel) {
			m_pCurrentModel->Cleanup();
			delete m_pCurrentModel;
		}
		m_IndexArray.RemoveAll();
		m_RevIndexArray.RemoveAll();
		m_SortIndexArray.RemoveAll();
		m_pSet = pSet;
		m_pCurrentModel = pSet ? new CEnumModel(pSet, FALSE, pModel, pvKey) : NULL;
		//clean image lists
		if (pSet) {
			m_ColumnMap = new CColumnMap(pSet);
			for (iCount = 0; iCount < GetColumnCount(); iCount++) {
				if (m_ColumnMap->GetColumnGroupCount() <= (DWORD)iCount) {
					DeleteColumn(iCount);
					//DeleteView(0, iCount);
					iCount--;
				} else {
					if (GetListViewArray(iCount)->Init() == -1) {
						//fail
					}
				}
			}
			for (; (DWORD)iCount < m_ColumnMap->GetColumnGroupCount(); iCount++) {
				SplitColumn(10 + 2);
				if (GetListViewArray(iCount)->Init() == -1) {
					//fail
				}
			}
			for (iCount = 0; (DWORD)iCount < m_ColumnMap->GetColumnGroupCount(); iCount++) {
				if (m_ColumnMap->GetColumnGroupCount() != (DWORD)iCount && iCount != 0)
					TrackColumnSize(((CCustomHeaderCtrl*)GetListViewArray(iCount - 1 )->GetListCtrl().GetHeaderCtrl())->GetIdealTotalColumnWidth(), iCount - 1);
				GetListViewArray(iCount)->Display();
			}
			SetHeaderSortItem(~0UL);
			RecalcLayout();
		}
	}
	HWND GetMarquee() { return m_hMarquee; }
	void SetMarquee(HWND hCur) { m_hMarquee = hCur; }
	HWND GetCurLock() { return m_hCur; }
	void SetCurLock(HWND hCur) { m_hCur = hCur; }
	LRESULT ReflectMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		int iCount;
		for (iCount = 0; iCount < GetColumnCount(); iCount++) {
			if (GetListViewArray(iCount)->GetSafeHwnd() != hWnd) GetListViewArray(iCount)->SendMessage(
							message, wParam, lParam);
		}
		return 0;
	}
	LRESULT ReflectNotifyMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		int iCount;
		for (iCount = 0; iCount < GetColumnCount(); iCount++) {
			if (GetListViewArray(iCount)->GetSafeHwnd() != hWnd) {
				((NMHDR*)lParam)->hwndFrom = GetListViewArray(iCount)->GetListCtrl().GetSafeHwnd();
				GetListViewArray(iCount)->SendMessage(
							message, wParam, lParam);
			}
		}
		return 0;
	}
	void ExpandCollapseTree(void* ParentEntry)
	{
		int iCount;
		void* lParam;
		int item;
		void* Entry = NULL;
		INT_PTR Counter;
		WORD bExpand = TRUE;
		lParam = ParentEntry;
		while (bExpand && (Entry = lParam) != NULL && m_pCurrentModel->GetRootParam() != lParam) {
			if (!m_ExpandedMap.Lookup(Entry, bExpand)) bExpand = FALSE;
			lParam = m_pCurrentModel->GetEntryParentParam(Entry);
		}
		if (bExpand && m_pCurrentModel->GetRootParam() != lParam) {
			//why is this needed or NULL entry returned?
			lParam = m_pCurrentModel->GetRootParam();
			Entry = lParam;
		}
		if (Entry && !m_ExpandedMap.Lookup(Entry, bExpand)) bExpand = FALSE;
		for (Counter = 0; Counter < m_pCurrentModel->GetChildCount(ParentEntry); Counter++) {
			if ((Entry = m_pCurrentModel->GetChildParam(ParentEntry, Counter)) != NULL) {
				for (iCount = 0; iCount < GetColumnCount(); iCount++) {
					if (bExpand) {
						if (m_RevIndexArray[(INT_PTR)Entry] == -1) {
							if ((item = GetListViewArray(iCount)->GetListCtrl().InsertItem(
													LVIF_TEXT | LVIF_PARAM,
													GetListViewArray(iCount)->GetListCtrl().GetItemCount(),
													LPSTR_TEXTCALLBACK,
													0, 0, 0, (LPARAM)Entry)) == -1) {
							}
						}
					} else {
						if ((item = (int)m_RevIndexArray[(INT_PTR)Entry]) != -1) {
							GetListViewArray(iCount)->GetListCtrl().DeleteItem(item);
						}
					}
				}
				ExpandCollapseTree(Entry);
			}
		}
	}
	void SetHeaderSortItem(DWORD dwOldCol)
	{
		// need to custom draw header control for Win 95 without IE3
		if (!PRU_CheckCommonControlsVersionMinimum(4, 7)) return;
		HDITEM HeaderItem = { 0 };
		DWORD dwSelCol = GetColumnMap()->GetColumnFromSortOrder(0);
		CHeaderCtrl* pHeaderCtrl = GetListViewArray(GetColumnMap()->GetColumnGroup(dwSelCol))->GetListCtrl().GetHeaderCtrl();
		if (!pHeaderCtrl) return;
		HeaderItem.mask =	HDI_FORMAT |
							(PRU_CheckCommonControlsVersionMinimum(6, 0) ?
							 0 : HDI_BITMAP);
		pHeaderCtrl->GetItem(GetColumnMap()->GetColumnIndex(dwSelCol),
							&HeaderItem);
		if (PRU_CheckCommonControlsVersionMinimum(6, 0)) {
			HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
			HeaderItem.fmt |= ( GetColumnMap()->GetColumnSortDirection(dwSelCol) ?
								HDF_SORTUP : HDF_SORTDOWN);
            
		} else {
			//insert new item
			if (HeaderItem.hbm != 0) {
				DeleteObject(HeaderItem.hbm);
				HeaderItem.hbm = 0;
			}
			HeaderItem.fmt |= HDF_BITMAP | HDF_BITMAP_ON_RIGHT;
			HeaderItem.hbm = (HBITMAP)LoadImage(AfxGetInstanceHandle(),
												MAKEINTRESOURCE(GetColumnMap()->GetColumnSortDirection(dwSelCol) ? 
																IDB_UP : IDB_DOWN),
												IMAGE_BITMAP, 0, 0, 
												LR_LOADMAP3DCOLORS);
		}
		pHeaderCtrl->SetItem(GetColumnMap()->GetColumnIndex(dwSelCol),
							&HeaderItem);
		//remove old item
		if (dwOldCol != -1 && dwOldCol != dwSelCol) {
			pHeaderCtrl = GetListViewArray(GetColumnMap()->GetColumnGroup(dwOldCol))->GetListCtrl().GetHeaderCtrl();
			pHeaderCtrl->GetItem(GetColumnMap()->GetColumnIndex(dwOldCol),
								&HeaderItem);
			if (PRU_CheckCommonControlsVersionMinimum(6, 0)) {
				HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
			} else {
				HeaderItem.fmt &= ~(HDF_BITMAP | HDF_BITMAP_ON_RIGHT);
				if (HeaderItem.hbm != 0) {
					DeleteObject(HeaderItem.hbm);
					HeaderItem.hbm = 0;
				}
			}
			pHeaderCtrl->SetItem(GetColumnMap()->GetColumnIndex(dwOldCol),
								&HeaderItem);
		}
		DWORD dwCount;
		if (dwOldCol != ~0UL) {
			for (dwCount = 1; dwCount < (DWORD)GetColumnMap()->GetColumnSortOrder(dwOldCol); dwCount++) {
				DWORD dwCol = GetColumnMap()->GetColumnFromSortOrder(dwCount);
				RECT rect;
				pHeaderCtrl = GetListViewArray(GetColumnMap()->GetColumnGroup(dwCol))->GetListCtrl().GetHeaderCtrl();
				pHeaderCtrl->GetItemRect(GetColumnMap()->GetColumnIndex(dwCol), &rect);
				pHeaderCtrl->InvalidateRect(&rect);
			}
		}
	}
	inline CColumnMap* GetColumnMap() { return m_ColumnMap; }
	INT_PTR GetColGroupIndex(CListCtrl* ListCtrl)
	{
		int iCount;
		for (iCount = 0; iCount < GetColumnCount(); iCount++) {
			if (ListCtrl->GetSafeHwnd() ==
				GetListViewArray(iCount)->GetSafeHwnd()) {
				return iCount;
			}
		}
		return -1;
	}
	CMapPtrToWord* GetExpandedMap() { return &m_ExpandedMap; }
	CArray<void*>* GetIndexArray() { return &m_IndexArray; }
	CArray<INT_PTR>* GetSortIndexArray() { return &m_SortIndexArray; }
	inline CEnumModel* GetCurrentModel() { return m_pCurrentModel; }
	CCustomListView* GetListViewArray(int iCount)
	{ return (CCustomListView*)GetPane(0, iCount); }
	CCustomListView* GetListByColIndex(DWORD dwIndex)
	{ return GetListViewArray(m_ColumnMap->GetColumnGroup(dwIndex)); }
	inline DisplaySet* GetDisplaySet() { return m_pSet; }
	DWORD GetNextSortItem(DWORD dwOrderIndex)
	{
		return GetColumnMap()->GetColumnFromSortOrder(
							GetColumnMap()->GetColumnSortOrder(dwOrderIndex) + 1);
	}
	void DoSort()
	{
		int iCount;
		qsort_s(m_SortIndexArray.GetData(), m_SortIndexArray.GetCount(), sizeof(INT_PTR), &CCustomMultiListView::SortFunction, this);
		for (iCount = 0; iCount < GetColumnCount(); iCount++) {
			if (GetListViewArray(iCount)->GetListCtrl().GetItemCount() != (int)m_SortIndexArray.GetCount())
				GetListViewArray(iCount)->GetListCtrl().SetItemCountEx((int)m_SortIndexArray.GetCount(),
								LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
			int iSubCount;
			UINT uiFlags;
			CRect Rect;
			GetListViewArray(iCount)->GetListCtrl().GetClientRect(&Rect);
			CRect hdrRect;
			GetListViewArray(iCount)->GetListCtrl().GetHeaderCtrl()->GetWindowRect(&hdrRect);
			//instead of HitTest could also get position and length of the scroll bar
			//GetListViewArray(iCount)->GetListCtrl().GetScrollRange(SB_VERT, &iBegin, &iEnd);
			for (	iSubCount = GetListViewArray(iCount)->GetListCtrl().HitTest(CPoint(Rect.left, Rect.top + hdrRect.Height()), &uiFlags);
					iSubCount < min(GetListViewArray(iCount)->GetListCtrl().HitTest(CPoint(Rect.left, Rect.bottom + hdrRect.Height() - 1), &uiFlags), m_SortIndexArray.GetCount()); iSubCount++) {
				GetListViewArray(iCount)->GetListCtrl().Update(iSubCount);
			}
		}
	}
	struct LVSearch
	{
		LPCTSTR szSearch;
		BOOL bPartial;
		INT_PTR* piBase;		
	};
	static int SearchFunction(void* pContext, const void* pvParam1, const void* pvParam2)
	{
		return ((CCustomMultiListView*)pContext)->SearchFunc((LVSearch*)pvParam1, (INT_PTR*)pvParam2);
	}
	int SearchFunc(LVSearch* pSearch, INT_PTR* piSortIndex)
	{
		DWORD dwIndex = GetColumnMap()->GetColumnFromSortOrder(0);
		int iRet = 0;
		DisplaySet* pSet = GetDisplaySet();
		if (dwIndex == -1) return 0; //unsorted
		BOOL bAscending = GetColumnMap()->GetColumnSortDirection(dwIndex);
		//must always resolve if a tree column present
		//tree search only must do each sibling group separately!
		if (GetColumnMap()->GetTreeColIndex() != -1) {
			//Resolve(m_IndexArray[lParam1], m_IndexArray[lParam2]);
		}
		//if sorted by coloring must search each color group separately!
		if (pSet->paDisplayColumns[dwIndex].pRendererInfo.pColorer) {
			return 0;
		}
		if (iRet == 0) {
			CString String;
			GetCurrentModel()->Render(
				&pSet->paDisplayColumns[dwIndex].pRendererInfo, (void*)m_IndexArray[*piSortIndex], &String);
			if (pSearch->bPartial) {
				iRet = _tcsinatcmp(pSearch->szSearch, String.Left((int)_tcslen(pSearch->szSearch)));
				if (iRet == 0 && piSortIndex != pSearch->piBase) {
					GetCurrentModel()->Render(
						&pSet->paDisplayColumns[dwIndex].pRendererInfo, (void*)m_IndexArray[*(piSortIndex - 1)], &String);
					iRet = (_tcsinatcmp(pSearch->szSearch, String.Left((int)_tcslen(pSearch->szSearch))) <= 0) ? -1 : 0;
				}
			}
			else iRet = _tcsinatcmp(pSearch->szSearch, String);
		}
		//parents always go above children
		if (!bAscending) {
			if (iRet < 0) iRet = 1; else if (iRet > 0) iRet = -1;
		}
		return iRet;
	}
	LRESULT FindItem(LPCTSTR psz, int iStart, BOOL bPartial, BOOL bWrap)
	{
		LVSearch lvs = { psz, bPartial, m_SortIndexArray.GetData() + iStart };
		//must sort color groups and tree siblings separately!
		INT_PTR* piFind = (INT_PTR*)bsearch_s(&lvs, m_SortIndexArray.GetData() + iStart,
					m_SortIndexArray.GetCount() - iStart, sizeof(INT_PTR),
					&CCustomMultiListView::SearchFunction, this);
		if (!piFind && bWrap && (iStart != 0)) {
			lvs.piBase = m_SortIndexArray.GetData();
			piFind = (INT_PTR*)bsearch_s(&lvs, m_SortIndexArray.GetData(), iStart, sizeof(INT_PTR),
					&CCustomMultiListView::SearchFunction, this);
		}
		return piFind ? (piFind - m_SortIndexArray.GetData()) : -1;
	}
protected:
	int OnCreate(LPCREATESTRUCT lpCreateStruct)
	{
		int iRet = CSplitterWndEx::OnCreate(lpCreateStruct);
		SetFont(GetParent()->GetFont());
		//must assign an m_pCurrentModel;
		//create column map
		return iRet;
	}
	afx_msg void OnDestroy()
	{
		while (GetColumnCount() > 1) {
			DeleteColumn(GetColumnCount() - 1);
		}
		CFrameWnd* pFrameWnd = EnsureParentFrame();	
		if (GetPane(0, 0) == pFrameWnd->GetActiveView())
			pFrameWnd->SetActiveView(NULL);
		DeleteView(0, 0);
		//delete windows before underlying data in case used before delete message received
		if (m_ColumnMap) {
			delete m_ColumnMap;
		}
		if (m_pCurrentModel) {
			m_pCurrentModel->Cleanup();
			delete m_pCurrentModel;
		}
		CSplitterWndEx::OnDestroy();
	}
	void PostNcDestroy()
	{
		// default for views is to allocate them on the heap
		//  the default post-cleanup is to 'delete this'.
		//  never explicitly call 'delete' on a view
		delete this;
	}
	afx_msg LRESULT OnUpdateUI(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		CArray<TreeViewChange*>* ViewChange;
		int iCount;
		//recurse through view change and update...
		if (m_pCurrentModel) {
			//m_pCurrentModel->OnUpdateDatabase();
			if ((ViewChange = m_pCurrentModel->GetViewChanges()) != NULL && ViewChange->GetCount()) {
				EndMenu();
				for (iCount = 0; iCount < ViewChange->GetCount(); iCount++) {
					UpdateTreeListChange(ViewChange->GetAt(iCount), 
						m_ColumnMap->GetTreeColIndex() != -1);
				}
				m_pCurrentModel->OnUpdateUI();
				m_SortIndexArray.RemoveAll();
				for (iCount = 0; iCount < m_IndexArray.GetCount(); iCount++) {
					m_SortIndexArray.Add(iCount);
				}
				DoSort();
			}
		}
		return 0;
	}
	BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
	{
		int iCount;
		for (iCount = 0; iCount < GetColumnCount(); iCount++) {
			if (((NMHDR*)lParam)->hwndFrom ==
				GetListViewArray(iCount)->GetSafeHwnd()) {
				break;
			}
		}
		if (iCount != GetColumnCount()) {
			if (((NMHDR*)lParam)->code == NM_RCLICK) {
			}
		}
		return CSplitterWndEx::OnNotify(wParam, lParam, pResult);
	}
	afx_msg LRESULT OnAppExpandCollapseTree(WPARAM /*wParam*/, LPARAM lParam)
	{
		ExpandCollapseTree((void*)lParam);
		return 0;
	}
	void UpdateTreeListChange(	TreeViewChange* ViewChange, BOOL bTree)
	{
		POSITION pos;
		void* pvKey;
		TreeViewChange* NextViewChange;
		ASSERT(ViewChange);
		if (ViewChange->pvOldKey == ViewChange->pvNewKey) {
			if (ViewChange->pvOldKey == (void*)~0) {
				//just process underlying list
			} else {
				//no change nothing to do
			}
		} else if (ViewChange->pvOldKey == (void*)~0) {
			//insert new item
			m_RevIndexArray.InsertAt((INT_PTR)ViewChange->pvNewKey, m_IndexArray.Add(ViewChange->pvNewKey));
		} else if (ViewChange->pvNewKey == (void*)~0) {
			m_IndexArray.RemoveAt(m_IndexArray.GetCount() - 1);
			//since numbering contiguous can remove all above values
			m_RevIndexArray.RemoveAt(min((INT_PTR)ViewChange->pvOldKey, m_RevIndexArray.GetCount() - 1));
		} else {
			//update item but missing subitem information...
			//swap to make deletion safe
			INT_PTR iSave = m_RevIndexArray[(INT_PTR)ViewChange->pvOldKey];
			m_RevIndexArray[(INT_PTR)ViewChange->pvOldKey] = m_RevIndexArray[(INT_PTR)ViewChange->pvNewKey];
			m_RevIndexArray[(INT_PTR)ViewChange->pvNewKey] = iSave;
		}
		pos = ViewChange->ChildMap.GetStartPosition();
		while (pos) {
			ViewChange->ChildMap.GetNextAssoc(pos, pvKey,
													(void*&)NextViewChange);
			UpdateTreeListChange(NextViewChange, bTree);
		}
	}
	//callback sort function
	static int SortFunction(void* pContext, const void* pvParam1, const void* pvParam2)
	{
		return ((CCustomMultiListView*)pContext)->SortFunc(*(void**)pvParam1, *(void**)pvParam2,
				((CCustomMultiListView*)pContext)->GetColumnMap()->GetColumnFromSortOrder(0));
	}
	int SortFunc(const void* pvParam1, const void* pvParam2, DWORD dwIndex)
	{
		int iRet = 0;
		DisplaySet* pSet = GetDisplaySet();
		if (dwIndex == -1) return 0; //unsorted
		BOOL bAscending = GetColumnMap()->GetColumnSortDirection(dwIndex);
		//must always resolve if a tree column present
		if (GetColumnMap()->GetTreeColIndex() != -1) {
			Resolve(m_IndexArray[(INT_PTR)pvParam1], m_IndexArray[(INT_PTR)pvParam2]);
		}
		//first sort on coloring
		if (pSet->paDisplayColumns[dwIndex].pRendererInfo.pColorer) {
			iRet = GetCurrentModel()->CompareColor(&pSet->paDisplayColumns[dwIndex].pRendererInfo, (void*)m_IndexArray[(INT_PTR)pvParam1], (void*)m_IndexArray[(INT_PTR)pvParam2]);
		}
		if (iRet == 0)
			iRet = GetCurrentModel()->Compare(
				&pSet->paDisplayColumns[dwIndex].pRendererInfo, (void*)m_IndexArray[(INT_PTR)pvParam1], (void*)m_IndexArray[(INT_PTR)pvParam2]);
		//parents always go above children
		if (!bAscending) {
			if (iRet < 0) iRet = 1; else if (iRet > 0) iRet = -1;
		}
		if (iRet == 0) {
			//recursively keep sorting on secondary, tertiary, etc, columns
			return this->SortFunc(	m_IndexArray[(INT_PTR)pvParam1], m_IndexArray[(INT_PTR)pvParam2],
									GetNextSortItem(dwIndex));
		} else {
			return iRet;
		}
	}
	void Resolve(void* & lParam1, void* & lParam2)
	{
		void* Entry1;
		void* Entry2;
		DWORD dwCounter;
		void* lParamParent1;
		void* lParamParent2;
		CArray<void*> ParamArray1;
		CArray<void*> ParamArray2;
		ParamArray1.InsertAt(0, lParamParent1 = lParam1);
		while (lParamParent1 && (Entry1 = lParamParent1) != NULL) {
			ParamArray1.InsertAt(0, lParamParent1 = GetCurrentModel()->GetEntryParentParam(Entry1));
		}
		if (lParamParent1) {
			ParamArray1.SetAt(0, lParamParent1 = 0);
		}
		ParamArray2.InsertAt(0, lParamParent2 = lParam2);
		while (lParamParent2 && (Entry2 = lParamParent2) != NULL) {
			ParamArray2.InsertAt(0, lParamParent2 = GetCurrentModel()->GetEntryParentParam(Entry2));
		}
		if (lParamParent2) {
			ParamArray2.SetAt(0, lParamParent2 = 0);
		}
		for (dwCounter = 0; dwCounter < (DWORD)(min(ParamArray1.GetCount(), ParamArray2.GetCount())); dwCounter++) {
			if (ParamArray1.GetAt(dwCounter) != ParamArray2.GetAt(dwCounter)) {
				break;
			}
		}
		if (dwCounter == (DWORD)(min(ParamArray1.GetCount(), ParamArray2.GetCount()))) {
			if (ParamArray1.GetCount() == ParamArray2.GetCount()) {
				lParam1 = lParam2 = 0;
			} else if (ParamArray1.GetCount() > ParamArray2.GetCount()) {
				lParam2 = (void*)~0;
			} else {
				lParam1 = (void*)~0;
			}
		} else {
			lParam1 = ParamArray1.GetAt(dwCounter);
			lParam2 = ParamArray2.GetAt(dwCounter);
		}
	}
	HWND m_hCur;
	HWND m_hMarquee;
	CArray<void*> m_IndexArray;
	CArray<INT_PTR> m_RevIndexArray;
	CArray<INT_PTR> m_SortIndexArray;
	CMapPtrToWord m_ExpandedMap;
	CColumnMap* m_ColumnMap;
	DisplaySet* m_pSet;
	CEnumModel* m_pCurrentModel;

	DECLARE_DYNCREATE(CCustomMultiListView)
	DECLARE_MESSAGE_MAP()
};