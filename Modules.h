#pragma once
#include <afxmt.h>
#include <afxtempl.h>
#include "utility.h"

//all classes/code should be further extracted and this should become
//  a set of only tree/list updating/comparison classes

//VS 2003 and earlier does not support _malloca/_freea
//  which have security enhancements
//VS 2003 SP1 is 1310 and VS 2005 is 1400
#if _MSC_VER < 1400
#define _malloca _alloca
#define _freea
#endif

//tree/list changes for insertions, deletions and updates
struct TreeViewChange
{
	void* pvOldKey;
	void* pvNewKey;
	CMapPtrToPtr ChildMap;
};

//Only a list: pvOldKey == ~0, pvNewKey == ~0
//No change: pvOldKey == pvNewKey, pvOldKey != ~0
//Insertion: pvOldKey == ~0, pvNewKey != ~0
//Deletion: pvOldKey != ~0, pvNewKey == ~0
//Update: pvOldKey != ~0, pvNewKey != ~0

class CGatherer;

typedef void (*RenderFunc)(CGatherer*, void*, void*, DWORD, void*);
typedef int (*ComparerFunc)(CGatherer*, void*, void*, void*, DWORD);
typedef void (*ColorerFunc)(CGatherer*, void*, void*, DWORD, COLORREF& clr);

struct RendererInfo
{
	//TCHAR* szCategory;
	//column display data
	RenderFunc pRenderer;
	ComparerFunc pComparer;
	void* pvArg; //optional caption or index
    DWORD dwType; //optional multiple display/sort methods per type though
	//  which is storage item and runtime modifiable
	CRuntimeClass* paGathererSets[10];
	ColorerFunc pColorer;
	//  first is default, rows may contain different numbers of elements
};

struct DisplayColumnInfo
{
	RendererInfo pCaptionRendererInfo;
	//RenderCorollator for multiple redundant Renderer sets
	RendererInfo pRendererInfo;
	RendererInfo pIconRenderer;//optional can be NULL
	RendererInfo pSelectedIconRenderer;//optional can be NULL
	//column display defaults
	BOOL bDefaultSelected;
	DWORD dwDefaultOrder;
	DWORD dwDefaultColGroup;
	DWORD dwDefaultSortOrder;//can be in sort order without selection
	BOOL bDefaultSortAscending;
};

typedef void (*ActionFunc)(CGatherer*, void*);

//supports caption (and possibly category) caption functions based on API
//  such as process suspend/resume
struct ContextActionItem
{
    LPCTSTR szCategory;
	RendererInfo pCaptionRenderer;
	CRuntimeClass* pGatherer;
	ActionFunc pAction;
	BOOL bIsMultiHandler;
};

//must figure out top level tree items from this
struct DisplaySet
{
	//TCHAR* szTreeCategory;
	RendererInfo pTreeRenderer;
	RendererInfo pTreeIconRenderer;//optional can be NULL
	RendererInfo pTreeSelectedIconRenderer;//optional can be NULL
	DWORD dwDefaultTreeColumnIndex;//~0 for hex dump
	DisplayColumnInfo paDisplayColumns[64];
	ContextActionItem paContextActionItems[10];
};

DisplaySet DisplaySets[];

//handle fetcher dependencies and view dependency reference counting
/*class CDataFetchManager
{
	CDataFetchManager() {}
	~CDataFetchManager() {}
	void LoadDataFetcher(TopLevelTreeItem* Fetcher)
	{
	}
	void UnloadDataFetcher(TopLevelTreeItem* Fetcher)
	{
		INT_PTR iCount;
		for (iCount = 0; iCount < m_FetcherList.GetCount(); iCount++) {
			if (Fetcher == m_FetcherList[iCount]) break;
		}
		if (iCount != m_FetcherList.GetCount()) {
			for (	iSubCount = 0;
					iSubCount < Fetcher->paDependencies; iSubCount++) {

			}
		}
	}
	CArray<CEnumModel*> m_FetcherList;
};*/

//gather: primary key (through enumeration), sort keys, display keys
//enumeration:
//enumerate key items
//currently single primary key typically an index integer or name string
//extra enumerable info data kept if needed
//data gatherers:
//gather data for key item
//data selectively retrieved if needed

//base class calls enumerate method expecting per item call back to base class
//base class precalculates gatherers from column map snapshot
//  and then executes them per item
//long ops should have periodic bailout checks based on column map
//  changes effecting the net gatherers
//changes noted by updating through base class TreeEntry/ListEntry interface
//the base class interface will manage the view change through this interface
//prerender display columns
//presort display columns

class CSorter
{
};

class CEnumModel;

enum eType
{
	eEnd,
	eByte,
	eWord,
	eDWord,
	eQWord,
	eString,
	eMultiString,
	eKeyValue,
	eBinary
};

class CGatherer : public CObject
{
public:
	CGatherer() : m_aTypes(NULL), m_apDependencies(NULL)  { }
	CGatherer(eType* paTypes, CRuntimeClass** apDependencies) : m_aTypes(paTypes), m_apDependencies(apDependencies) { }
	virtual void GetDependencies(CArray<CRuntimeClass*> & DependencyArray)
	{
		DWORD dwCount;
		if (m_apDependencies == NULL) return;
		for (dwCount = 0; m_apDependencies[dwCount] != NULL; dwCount++) {
			DependencyArray.Add(m_apDependencies[dwCount]);
		}
	}
	virtual void* GetItemData(void* pvKey, int iIndex)
	{
		DWORD dwIndex = m_ValueIndexes[(DWORD_PTR)pvKey][iIndex];
		switch (m_aTypes[iIndex]) {
		case eDWord:
			return (dwIndex == ~0UL) ? NULL : &m_DWordValues[dwIndex];
		case eString:
			return (dwIndex == ~0UL) ? NULL : (void*)m_StrValues[dwIndex].GetString();
		case eMultiString:
			return (dwIndex == ~0UL) ? NULL : (void*)&m_MultiStrValues[dwIndex];
		case eKeyValue:
			return (dwIndex == ~0UL) ? NULL : (void*)&m_KeyValues[dwIndex];
		case eBinary:
			return (dwIndex == ~0UL) ? NULL : &m_BinaryValues[dwIndex];
		default:
			throw;
		}
	}
	virtual BOOL CompareItem(void* pvKey, CGatherer* pCompareGatherer, CDWordArray & IndexMismatches)
	{
		CCopyByteArray* paBytes;
		BOOL bMatch = TRUE;
		DWORD dwCount;
		for (dwCount = 0; m_aTypes[dwCount] != eEnd; dwCount++) {
			DWORD dwIndex = m_ValueIndexes[(DWORD_PTR)pvKey][dwCount];
			switch (m_aTypes[dwCount]) {
			case eDWord:
				if (m_DWordValues[dwIndex] != *((DWORD*)pCompareGatherer->GetItemData(pvKey, dwCount))) {
					IndexMismatches.Add(dwCount);
					bMatch = FALSE;
				}
				break;
			case eString:
				if (m_StrValues[dwIndex].Compare((LPCTSTR)pCompareGatherer->GetItemData(pvKey, dwCount)) != 0) {
					IndexMismatches.Add(dwCount);
					bMatch = FALSE;
				}
				break;
			case eMultiString:
				if (m_MultiStrValues[dwIndex].Compare(*((CCopyStringArray*)pCompareGatherer->GetItemData(pvKey, dwCount))) != 0) {
					IndexMismatches.Add(dwCount);
					bMatch = FALSE;
				}
				break;
			case eKeyValue:
				if (m_KeyValues[dwIndex].Compare(*((CCopyKVArray*)pCompareGatherer->GetItemData(pvKey, dwCount))) != 0) {
					IndexMismatches.Add(dwCount);
					bMatch = FALSE;
				}
				break;
			case eBinary:
				paBytes = (CCopyByteArray*)pCompareGatherer->GetItemData(pvKey, dwCount);
				if (m_BinaryValues[dwIndex].GetSize() != paBytes->GetSize() ||
					memcmp(m_BinaryValues[dwIndex].GetData(), paBytes->GetData(), m_BinaryValues[dwIndex].GetSize()) != 0) {
					IndexMismatches.Add(dwCount);
					bMatch = FALSE;
				}
				break;
			default:
				throw;
			}
		}
		return bMatch;
	}
	virtual CString GetTreePath(CEnumModel *pModel, void* pvKey) { return _T(""); }
	void AddItem(CEnumModel* pModel, CRuntimeClass* pRuntimeClass, CCopyDWordArray & NewValues);
	virtual void SetItemData(int iIndex, void* pvValue, CCopyDWordArray & NewValues)
	{
		switch (m_aTypes[iIndex]) {
		case eDWord:
			NewValues.Add(pvValue ? (DWORD)m_DWordValues.Add(*((DWORD*)pvValue)) : ~0UL);
			break;
		case eString:
			NewValues.Add(pvValue ? (DWORD)m_StrValues.Add((LPCTSTR)pvValue) : ~0UL);
			break;
		case eMultiString:
			NewValues.Add(pvValue ? (DWORD)m_MultiStrValues.Add(*((CCopyStringArray*)pvValue)) : ~0UL);
			break;
		case eKeyValue:
			NewValues.Add(pvValue ? (DWORD)m_KeyValues.Add(*((CCopyKVArray*)pvValue)) : ~0UL);
			break;
		case eBinary:
			NewValues.Add(pvValue ? (DWORD)m_BinaryValues.Add(*((CCopyByteArray*)pvValue)) : ~0UL);
			break;
		default:
			throw;
		}
	}
	virtual void OnEnumerate(CEnumModel* pModel, CGatherer* pDepGatherer, CArray<void*>* pDepKeys) = 0;
	/*{
		DWORD dwType = m_pArgs->ArgValues[iIndex].dwType;
		DWORD dwIndex = m_ValueIndexes[(DWORD_PTR)pvKey][iIndex];
	}*/
protected:
	eType* m_aTypes;
	DWORD m_dwTypeCount;
	CRuntimeClass** m_apDependencies;
	DWORD m_dwDependencies;
	CArray<CCopyDWordArray, CCopyDWordArray&> m_ValueIndexes;

	CArray<CCopyByteArray, CCopyByteArray&> m_BinaryValues;
	CArray<CCopyStringArray, CCopyStringArray&> m_MultiStrValues;
	CArray<CCopyKVArray, CCopyKVArray&> m_KeyValues;
	CStringArray m_StrValues;
	CDWordArray m_DWordValues;
};

#define MAKEGATHERER(cname) cname() : CGatherer(m_aTypes, m_apDependencies) { }\
	private:\
	static eType m_aTypes[];\
	static CRuntimeClass* m_apDependencies[];

//must not contain any UI elements
class CEnumModel
{
public:
	//needs to create update thread
	CEnumModel(DisplaySet* pSet, BOOL bTree, CEnumModel* pTreeModel = NULL, void* pvTreeKey = NULL)
	{
		DWORD dwCount;
		m_pTreeModel = pTreeModel;
		m_pvTreeKey = pvTreeKey;
		if (bTree) {
			AddGatherers(m_GathererRuntimes, &pSet->pTreeRenderer);
			AddGatherers(m_GathererRuntimes, &pSet->pTreeIconRenderer);
			AddGatherers(m_GathererRuntimes, &pSet->pTreeSelectedIconRenderer);
		} else {
			for (dwCount = 0; dwCount < 32; dwCount++) {
				AddGatherers(m_GathererRuntimes, &pSet->paDisplayColumns[dwCount].pCaptionRendererInfo);
				AddGatherers(m_GathererRuntimes, &pSet->paDisplayColumns[dwCount].pRendererInfo);
				AddGatherers(m_GathererRuntimes, &pSet->paDisplayColumns[dwCount].pIconRenderer);
				AddGatherers(m_GathererRuntimes, &pSet->paDisplayColumns[dwCount].pSelectedIconRenderer);
			}
			for (dwCount = 0; dwCount < 10; dwCount++) {
				AddGatherers(m_GathererRuntimes, &pSet->paContextActionItems[dwCount].pCaptionRenderer);
			}
		}
	}
	void AddGatherers(CArray<CRuntimeClass*> & GatherArray, RendererInfo* pRenderInfo)
	{
		DWORD dwCount;
		DWORD dwSubCount;
		for (dwCount = 0; dwCount < 10; dwCount++) {
			if (pRenderInfo->paGathererSets[dwCount]) {
				for (dwSubCount = 0; dwSubCount < (DWORD)GatherArray.GetCount(); dwSubCount++) {
					if (GatherArray[dwSubCount]->m_pfnCreateObject == pRenderInfo->paGathererSets[dwCount]->m_pfnCreateObject) break;
				}
				if (dwSubCount == (DWORD)GatherArray.GetCount()) GatherArray.Add(pRenderInfo->paGathererSets[dwCount]);
			}
		}
	}
	void Color(RendererInfo* pRI, void* pvKey, COLORREF& clr)
	{
		if (pRI->pColorer) pRI->pColorer(pRI->paGathererSets[0] ? GetGatherer(pRI->paGathererSets[0]) : NULL, pvKey, pRI->pvArg, pRI->dwType, clr);
	}
	void Render(RendererInfo* pRI, void* pvKey, void* pvOut)
	{
		if (pRI->pRenderer && (m_OldGatherers.GetCount() || !pRI->paGathererSets[0] || pRI->dwType == 0)) pRI->pRenderer(pRI->paGathererSets[0] ? GetGatherer(pRI->paGathererSets[0]) : NULL, pvKey, pRI->pvArg, pRI->dwType, pvOut);
	}
	int CompareColor(RendererInfo* pRI, void* pvKey1, void* pvKey2)
	{
		COLORREF clr1;
		COLORREF clr2;
		if (pRI->pColorer) {
			pRI->pColorer(pRI->paGathererSets[0] ? GetGatherer(pRI->paGathererSets[0]) : NULL, pvKey1, pRI->pvArg, pRI->dwType, clr1);
			pRI->pColorer(pRI->paGathererSets[0] ? GetGatherer(pRI->paGathererSets[0]) : NULL, pvKey2, pRI->pvArg, pRI->dwType, clr2);
			return (clr1 == clr2) ? 0 : ((clr1 < clr2) ? -1 : 1);
		} else return 0;
	}
	int Compare(RendererInfo* pRI, void* pvKey1, void* pvKey2)
	{
		return (!pRI->pComparer) ? 0 : pRI->pComparer(pRI->paGathererSets[0] ? GetGatherer(pRI->paGathererSets[0]) : NULL, pvKey1, pvKey2, pRI->pvArg, pRI->dwType);
	}
	CGatherer* GetGatherer(CRuntimeClass* pRuntimeClass)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < (DWORD)m_OldGatherers.GetCount(); dwCount++) {
			if (m_OldGatherers.GetAt(dwCount)->IsKindOf(pRuntimeClass)) {
				return m_OldGatherers.GetAt(dwCount);
			}
		}
		return NULL;
	}
	CGatherer* GetBuildGatherer(CRuntimeClass* pRuntimeClass)
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < (DWORD)m_Gatherers.GetCount(); dwCount++) {
			if (m_Gatherers.GetAt(dwCount)->IsKindOf(pRuntimeClass)) {
				return m_Gatherers.GetAt(dwCount);
			}
		}
		//if not found here try in tree model
		return m_pTreeModel ? m_pTreeModel->GetBuildGatherer(pRuntimeClass) : NULL;
	}
	virtual ~CEnumModel() { Cleanup(); }
	void Cleanup() {
		if (m_ViewUpdateCS.Lock()) {
			ClearViewChange();
			RemoveGatherers(m_OldGatherers);
			RemoveGatherers(m_Gatherers);
			m_Keys.RemoveAll();
			m_NextOldKeys.RemoveAll();
			m_OldKeys.RemoveAll();
			m_ViewUpdateCS.Unlock();
		}
	}
	//update thread needs following GUI info:
	//columns displayed or very likely to be displayed (based on cpu)
	//indexes of items that are displayed
	//  or very likely to be displayed (based on cpu)
	//update thread should be sent a message whenever columns
	void OnUpdateDatabase()
	{
		CArray<CRuntimeClass*> DependencyArray;
		CArray<void*> DepKeys;
		DWORD dwCount;
		//if have gatherers then no update has taken place so either do nothing
		//or dispose gatherers and gather again
		//m_Gatherers.RemoveAll(); ClearViewChange();
		if (m_ViewUpdateCS.Lock()) {
			if (m_Gatherers.GetCount()) {
				m_ViewUpdateCS.Unlock();
				return;
			}
			//create and go through all gatherers in dependency order
			m_OldKeys.Transfer(m_NextOldKeys);
			m_Gatherers.SetSize(m_GathererRuntimes.GetCount());
			for (dwCount = 0; dwCount < (DWORD)m_GathererRuntimes.GetCount(); dwCount++) {
				m_Gatherers.SetAt(dwCount, (CGatherer*)m_GathererRuntimes.GetAt(dwCount)->CreateObject());
				m_Gatherers.GetAt(dwCount)->GetDependencies(DependencyArray);
				if (m_pTreeModel)
					DepKeys.Add(m_pvTreeKey);
				else if (DependencyArray.GetCount()) {
					POSITION pos = m_OldKeys.GetStartPosition();
					void* pvKey;
					void* pvParentKey;
					while (pos) {
						m_OldKeys.GetNextAssoc(pos, pvKey, pvParentKey);
						DepKeys.Add(pvKey);
					}
				}
				m_Gatherers.GetAt(dwCount)->OnEnumerate(this, DependencyArray.GetCount() ? GetBuildGatherer(DependencyArray[0]) : NULL, &DepKeys);
			}
			//go through all old gatherer items and delete unused
			if (m_OldKeys.GetCount()) {
				POSITION pos = m_OldKeys.GetStartPosition();
				void* pvKey;
				void* pvParentKey;
				while (pos) {
					m_OldKeys.GetNextAssoc(pos, pvKey, pvParentKey);
					if (pvParentKey != (void*)~1) {
						TreeViewChange* NewViewChange = new TreeViewChange;
						if (NewViewChange) {
							NewViewChange->pvOldKey = pvKey;
							NewViewChange->pvNewKey = (void*)~0;
							m_TreeViewChanges.Add(NewViewChange);
						}
					}
				}
				m_OldKeys.RemoveAll();
			}
			m_ViewUpdateCS.Unlock();
		}
	}
	void OnItemNew(CRuntimeClass* pRuntimeClass, void* pvKey, CArray<void*, void*&>* pChildren = NULL)
	{
		void* pvParentKey;
		DWORD dwCount;
		DWORD dwSubCount;
		if (m_NextOldKeys.Lookup(pvKey, pvParentKey)) return;
		if (m_OldKeys.Lookup(pvKey, pvParentKey)) {
			//all columns with this gatherer must be compared
			//if (pOldGatherer->Compare() != 0) {
				TreeViewChange* NewViewChange = new TreeViewChange;
				if (NewViewChange) {
					NewViewChange->pvOldKey = pvKey;
					NewViewChange->pvNewKey = pvKey;
					if (pChildren) {
						for (dwCount = 0; dwCount < (DWORD)pChildren->GetCount(); dwCount++) {
							for (dwSubCount = 0; dwSubCount < (DWORD)m_TreeViewChanges.GetCount(); dwSubCount++) {
								if (m_TreeViewChanges[dwSubCount]->pvNewKey == pChildren->GetAt(dwCount)) {
									NewViewChange->ChildMap.SetAt(pChildren->GetAt(dwCount), m_TreeViewChanges[dwSubCount]);
									m_TreeViewChanges.RemoveAt(dwSubCount);
								}
							}
							m_NextOldKeys.SetAt(pChildren->GetAt(dwCount), pvKey);
						}
					}
					m_TreeViewChanges.Add(NewViewChange);
				}
			//}
			m_OldKeys.SetAt(pvKey, (void*)~1);
		} else {
			TreeViewChange* NewViewChange = new TreeViewChange;
			if (NewViewChange) {
				NewViewChange->pvOldKey = (void*)~0;
				NewViewChange->pvNewKey = pvKey;
				if (pChildren) {
					for (dwCount = 0; dwCount < (DWORD)pChildren->GetCount(); dwCount++) {
						for (dwSubCount = 0; dwSubCount < (DWORD)m_TreeViewChanges.GetCount(); dwSubCount++) {
							if (m_TreeViewChanges[dwSubCount]->pvNewKey == pChildren->GetAt(dwCount)) {
								NewViewChange->ChildMap.SetAt(pChildren->GetAt(dwCount), m_TreeViewChanges[dwSubCount]);
								m_TreeViewChanges.RemoveAt(dwSubCount);
							}
						}
						m_NextOldKeys.SetAt(pChildren->GetAt(dwCount), pvKey);
					}
				}
				m_TreeViewChanges.Add(NewViewChange);
			}
		}
		m_NextOldKeys.SetAt(pvKey, (void*)~0);
	}
	virtual BOOL IsEmpty() { return m_Keys.GetCount() == 0; }

	//must call ClearViewChange
	void OnUpdateUI()
	{
		if (m_ViewUpdateCS.Lock()) {
			if (m_TreeViewChanges.GetCount() == 0) {
				RemoveGatherers(m_Gatherers);
			} else {
				ClearViewChange();
				RemoveGatherers(m_OldGatherers);
				m_OldGatherers.Transfer(m_Gatherers);
				m_Keys.Copy(m_NextOldKeys);
			}
			m_ViewUpdateCS.Unlock();
		}
	}
	CArray<TreeViewChange*>* GetViewChanges() { return &m_TreeViewChanges; }
	virtual void OnColumnAction(ContextActionItem* ActionItem, CArray<void*, void*&>* pKeys)
	{
		if (m_ViewUpdateCS.Lock()) {
			if (ActionItem->bIsMultiHandler)
				ActionItem->pAction(GetGatherer(ActionItem->pGatherer), pKeys);
			else {
				DWORD dwCount;
				for (dwCount = 0; dwCount < (DWORD)pKeys->GetCount(); dwCount++) {
					ActionItem->pAction(GetGatherer(ActionItem->pGatherer), pKeys->GetAt(dwCount));
				}
			}
			m_ViewUpdateCS.Unlock();
		}
	}
	//tree only related functions
	virtual void* GetEntryParentParam(void* pvKey)
	{
		void* rValue;
		return m_Keys.Lookup(pvKey, rValue) && ((INT_PTR)rValue != ~0)? rValue : NULL;
	}
	virtual void* GetRootParam()
	{
		void* rKey;
		void* rValue;
		POSITION pos = m_Keys.GetStartPosition();
		m_Keys.GetNextAssoc(pos, rKey, rValue);
		while (m_Keys.Lookup(rKey, rValue) && ((INT_PTR)rValue != ~0))
		{ rKey = rValue; }
		return rKey;
	}
	virtual void* GetChildParam(void* pvKey, INT_PTR iIndex)
	{
		INT_PTR iCount = 0;
		void* rKey;
		void* rValue;
		POSITION pos = m_Keys.GetStartPosition();
		while (pos) {
			m_Keys.GetNextAssoc(pos, rKey, rValue);
			if (rValue == pvKey && iCount++ == iIndex) return rKey;
		}
		return NULL;
	}
	virtual BOOL HasChild(void* pvKey)
	{
		void* rKey;
		void* rValue;
		POSITION pos = m_Keys.GetStartPosition();
		while (pos) {
			m_Keys.GetNextAssoc(pos, rKey, rValue);
			if (rValue == pvKey) return TRUE;
		}
		return FALSE;
	}
	virtual DWORD GetDepth(void* pvKey)
	{
		void* rKey;
		void* rValue;
		DWORD dwDepth = 0;
		rKey = pvKey;
		while (m_Keys.Lookup(rKey, rValue) && ((INT_PTR)rValue != ~0))
		{ rKey = rValue; dwDepth++; }
		return dwDepth;
	}
	virtual INT_PTR GetChildCount(void* pvKey)
	{
		INT_PTR iCount = 0;
		void* rKey;
		void* rValue;
		POSITION pos = m_Keys.GetStartPosition();
		while (pos) {
			m_Keys.GetNextAssoc(pos, rKey, rValue);
			if (rValue == pvKey) iCount++;
		}
		return iCount;
	}
	CEnumModel* GetTreeModel() { return m_pTreeModel; }
protected:
	void RemoveGatherers(CArray<CGatherer*> & pGatherers)
	{
		for (DWORD dwCount = 0; dwCount < (DWORD)pGatherers.GetCount(); dwCount++) {
			delete pGatherers.GetAt(dwCount);
		}
		pGatherers.RemoveAll();
	}
	void ClearViewChange()
	{
		DWORD dwCount;
		for (dwCount = 0; dwCount < (DWORD)m_TreeViewChanges.GetCount(); dwCount++) {
			DeleteViewChange(m_TreeViewChanges[dwCount]);
		}
		m_TreeViewChanges.RemoveAll();
	}
	void DeleteViewChange(TreeViewChange* ViewChange)
	{
		POSITION pos;
		void* pvOldKey;
		TreeViewChange* NextViewChange;
		if (ViewChange) {
			if (ViewChange->ChildMap.GetCount()) {
				pos = ViewChange->ChildMap.GetStartPosition();
				while (pos) {
					ViewChange->ChildMap.GetNextAssoc(pos, (void*&)pvOldKey,
															(void*&)NextViewChange);
					DeleteViewChange(NextViewChange);
				}
			}
			delete ViewChange;
		}
	}
	CEnumModel* m_pTreeModel;
	void* m_pvTreeKey;
	//view changes used in thread safe manner
	CArray<TreeViewChange*> m_TreeViewChanges;
	CArray<CRuntimeClass*> m_GathererRuntimes;
	CCopyArray<CGatherer*> m_Gatherers;
	CCopyArray<CGatherer*> m_OldGatherers;
	// single parent only
	CTransferMapPtrToPtr m_Keys;
	CTransferMapPtrToPtr m_OldKeys;
	CTransferMapPtrToPtr m_NextOldKeys;
	CCriticalSection m_ViewUpdateCS;
};

	/*void MarkChildrenForDeletion(	TreeViewChange* ViewChange,
									TreeViewEntry* ViewEntry)
	{
		POSITION pos;
		TreeViewChange* NextViewChange;
		TreeViewEntry* NextOldTreeEntry;
		TKey Key;
		pos =	TreeViewEntry ?
				TreeViewEntry->Children.GetStartPosition() : NULL;
		//view should only mark visible or probabably visible for deletion!
		while (pos) {
			OldTreeEntry->Children.GetNextAssoc(pos, Key,
												(void*&)NextOldTreeEntry);
			NextViewChange = new TreeViewChange;
			NextViewChange->PtrChildren = new CPtrMap;
			NextViewChange->pvOldKey = (pvOldKey)NextOldTreeEntry;
			NextViewChange->pvNewKey = NULL;
			m_TreeViewChange->PtrChildren->SetAt(	NextOldTreeEntry,
													(void*&)NextViewChange);
		}
	}
	//view gets to choose the active renderers, sorters and filters
	//  if any of these change must perform model update
	void OnUpdateModel(	CArray<RendererInfo*> Renderers,
						CArray<SorterInfo*> Sorters,
						CArray<FilterInfo*> Filters)
	{
		//call OnItemEnum
		//Mark top level and its children for deletion
		m_TreeViewChange = new TreeViewChange;
		m_TreeViewChange->PtrChildren = new CPtrMap;
		m_TreeViewChange->pvOldKey = m_TreeViewEntry;
		m_TreeViewChange->pvNewKey = NULL;
		return OnItemEnum(	NULL, NULL, 0, NULL, m_TreeViewChange, 
							m_TreeViewEntry, Renderers, Sorters, Filters);
	}
	//return TRUE if data should be discarded for no change
	BOOL OnItemEnum(TKey* Key, TItem* Item, unsigned int uiImplicitIndex,
					pvOldKey pvOldKey, TreeViewChange* ViewChange,
					TreeViewEntry* ViewEntry, CArray<RendererInfo*> Renderers,
					CArray<SorterInfo*> Sorters, CArray<FilterInfo*> Filters)
	{
		TreeViewChange* NextViewChange = NULL;
		TreeViewEntry* NextTreeEntry;
		TreeViewEntry* NextOldTreeEntry;
		//run enum item filter
		NextTreeEntry = new TreeViewEntry;
		NextTreeEntry->Entry.bSelected = FALSE;
		NextTreeEntry->Entry.hIcon = NULL;
		NextTreeEntry->Entry.hSelectedIcon = NULL;
		NextTreeEntry->Entry.EntryText = NULL;
		NextTreeEntry->Entry.SortItem = NULL;
		NextTreeEntry->Entry.RenderDataArray = NULL;
		NextTreeEntry->Entry.RenderDataArray[0] = Key;
		NextTreeEntry->bExpanded = FALSE;
		NextTreeEntry->ParentEntry = NULL;
		//only if fetcher used for renderer
		//if (non key Item fetcher used for renderer) NextTreeEntry->Entry.RenderDataArray[1] = Item;
		//if (index used for renderer) NextTreeEntry->Entry.RenderDataArray[2] = uiImplicitIndex;
		//run gatherers
		//run filters
		if (m_TreeViewEntry &&
			m_TreeViewEntry->Children.Lookup(Key, (void*&)NextOldTreeEntry) &&
			CompareRenderData(NextTreeEntry, NextOldTreeEntry) == 0) {
		}
		if (!m_TreeViewChange ||
			!m_TreeViewChange->PtrChildren->Lookup(	NextOldTreeEntry,
													(void*&)NextViewChange)) {
			NextOldTreeEntry = NULL;
			NextViewChange = new TreeViewChange;
			NextViewChange->PtrChildren = new CPtrMap;
			NextViewChange->pvOldKey = (pvOldKey)NextOldTreeEntry;
		} else if (NextOldTreeEntry->Index == ulIndex) {
			//no change
			//streamline the allocations...
			delete [] NextTreeEntry->Name;
			delete NextTreeEntry;
			NextViewChange->pvNewKey = NextViewChange->pvOldKey;
            NextTreeEntry = NextOldTreeEntry;
			NextTreeEntry->bProtected = TRUE;
		} else {
			//protect from orphaned pointers
			m_DeletionArray.Add(NextOldTreeEntry);
		}
		NextTreeEntry->Children.SetAt(	CString(NextTreeEntry->Entry.EntryText),
									NextTreeEntry);
		NextViewChange->pvNewKey = (pvOldKey)NextTreeEntry;
		if (!ViewChange) {
			ViewChange = NextViewChange; //delayed or bool needed
			//because of the else clause should not be executed in
			//  this special case
		} else {
			ViewChange->PtrChildren->SetAt(	NextOldTreeEntry ?
											NextOldTreeEntry :
											NextTreeEntry,
											(void*&)NextViewChange);
		}
		MarkChildrenForDeletion(ViewChange, ViewEntry);
		if (CheckRecurseFilter(Item)) {
			bChange |= OnEnumerateItems(pvOldKey);
			//sorting
		}
		if (!bChange) delete NextTreeEntry;
	}*/