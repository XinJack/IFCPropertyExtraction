#include "stdafx.h"
#include "ifcviewer.h"
#include "LeftPane.h"
#include "generic.h"
#include "unit.h"

// 添加的文件输入头
#include <iostream>
#include <fstream>
using namespace std;

#include "ifcengine\include\ifcengine.h"

CWnd	* glLeftPane = nullptr;
extern	bool	newFileName;
extern	STRUCT__SELECTABLE__TREEITEM	* baseTreeItem;
extern	STRUCT__IFC__OBJECT				* ifcObjectsLinkedList,
										* highLightedIfcObject;
extern	int_t							globalIfcModel;

// CLeftPane

IMPLEMENT_DYNCREATE(CLeftPane, CTreeView)

CLeftPane::CLeftPane(CWnd* pParent /*=NULL*/)
{
	pParent = pParent;
}

CLeftPane::~CLeftPane()
{
}

void CLeftPane::DoDataExchange(CDataExchange* pDX)
{
	CTreeView::DoDataExchange(pDX);
}

#define ID_ATTR_EXTRACT                 0xE139 // 很奇怪，需要在afxres.h和本文件同时定义ID_ATTR_EXTRACT才能使用

BEGIN_MESSAGE_MAP(CLeftPane, CTreeView)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnRClick)
	ON_NOTIFY_REFLECT(TVN_GETINFOTIP, OnGetInfoTip)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnBeforeExpand)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelectionChanged) 

	ON_COMMAND(ID_ATTR_EXTRACT, &CLeftPane::OnAttrExtract)
END_MESSAGE_MAP()

// 递归获取属性
void extractAttr(CTreeCtrl *treePtr, HTREEITEM node, CStdioFile *pFile, int tabNum)
{
	// 当前节点拥有子节点
	if(treePtr->ItemHasChildren(node))
	{
		// 先将当前节点展开（这里这一步好像是必须的，不然未展开的节点属性无法被获取）
		treePtr->Expand(node, TVE_EXPAND);
		
		// 遍历所有的子节点
		HTREEITEM childNode = treePtr->GetChildItem(node);
		while(childNode != NULL)
		{
			// 获取子节点文字
			CString text = treePtr->GetItemText(childNode);
			text.Replace(L"\n", L""); // 将原有的换行符去除
			for(int i = 0; i < tabNum; ++i)
			{
				pFile->WriteString(L"	");
			}
			pFile->WriteString(text);
			pFile->WriteString(L"\n");
			extractAttr(treePtr, childNode, pFile, tabNum + 1);
			childNode = treePtr->GetNextItem(childNode, TVGN_NEXT);
		}
	}
}

// 属性提取响应函数
void CLeftPane::OnAttrExtract()
{
	// 代码 
	//MessageBox(L"测试");
	
	// 确保在文件输出CString的时候中文不会出现乱码
	CString tmp = L"";
	tmp = setlocale(LC_CTYPE, "chs");

	// 保存对话框
	BOOL isOpen = FALSE;
	CString filter = L"文件(*.txt)";
	CFileDialog openFileDialog(isOpen, NULL, NULL, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, filter, NULL);
	INT_PTR result = openFileDialog.DoModal();
	CString filePath;
	if(result = IDOK)
	{
		// 保存文件路径
		filePath = openFileDialog.GetPathName() + L".txt";
		
		CStdioFile *pFile = new CStdioFile(filePath, CFile::modeNoTruncate | CFile::modeCreate | CFile::modeWrite);

		// 树结构的指针
		CTreeCtrl *treePtr = &GetTreeCtrl();
		// 第一个根节点
		HTREEITEM root = treePtr->GetRootItem();
		CString rootText = treePtr->GetItemText(root);
		pFile->WriteString(rootText + L"\n");
		extractAttr(treePtr, root, pFile, 1);
		// 循环获取其他根节点
		HTREEITEM sibling = treePtr->GetNextSiblingItem(root);
		while(sibling != NULL)
		{
			CString siblingText = treePtr->GetItemText(sibling);
			pFile->WriteString(siblingText + L"\n");
			extractAttr(treePtr, sibling, pFile, 1);
			sibling = treePtr->GetNextSiblingItem(sibling);
		}

		pFile->Close();
		MessageBox(L"属性提取完成");
	}else
	{
		return;
	}
}

CString	NestedPropertyValue(int_t ifcEntityInstance, wchar_t * propertyName, int_t propertyType, int_t * ifcAggregate)
{
	CString	rValue = 0;

	if	(ifcAggregate) {
		ASSERT(ifcEntityInstance == 0  &&  propertyName == 0  &&  propertyType == 0);
		engiGetAggrType(ifcAggregate, &propertyType);
	} else {
		ASSERT(ifcEntityInstance  &&  propertyName  &&  propertyType);
	}

	wchar_t	* buffer = nullptr, buff[512];

	int_t	ifcInstance, lineNo, * ifcInstanceAggr, ifcInstanceAggrCnt, integer;

	wchar_t	* nominalValue = nullptr,
			* typePath = nullptr;
	int_t	* nominalValueADB = nullptr;

	switch  (propertyType) {
		case  sdaiADB:
			sdaiGetAttrBN(ifcEntityInstance, (char*) propertyName, sdaiUNICODE, &nominalValue);
			if	(nominalValue) {
				sdaiGetAttrBN(ifcEntityInstance, (char*) propertyName, sdaiADB, &nominalValueADB);
				typePath = (wchar_t*) sdaiGetADBTypePath(nominalValueADB, sdaiUNICODE);
			}

			if	(nominalValue) {
				rValue += typePath;
				rValue += " (";
				rValue += nominalValue;
				rValue += ")";
			} else {
				rValue += "?";
			}
			break;
		case  sdaiAGGR:
			rValue += "(";
			ifcInstanceAggr = 0;
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char *) propertyName, sdaiAGGR, &ifcInstanceAggr);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiAGGR, &ifcInstanceAggr);
			}
			if	(ifcInstanceAggr) {
				ifcInstanceAggrCnt = sdaiGetMemberCount(ifcInstanceAggr);

				if	(ifcInstanceAggrCnt > 0) {
					rValue += NestedPropertyValue(0, 0, 0, ifcInstanceAggr);
					if	(ifcInstanceAggrCnt > 1) {
						rValue += ", ...";
					}
				} else {
					ASSERT(ifcInstanceAggrCnt == 0);
					rValue += " ?";
				}
			} else {
				rValue += "???";
			}
			rValue += ")";
			break;
		case  sdaiBINARY:
		case  sdaiBOOLEAN:
		case  sdaiENUM:
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char*) propertyName, sdaiUNICODE, &buffer);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiUNICODE, &buffer);
			}

			if	(buffer  &&  buffer[0]) {
				rValue += buffer;
			} else {
				rValue += "?";
			}
			break;
		case  sdaiINSTANCE:
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char *) propertyName, sdaiINSTANCE, &ifcInstance);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiINSTANCE, &ifcInstance);
			}
			if	(ifcInstance) {
				lineNo =internalGetP21Line(ifcInstance);
				rValue += "#";
#ifdef	WIN64
				_i64tow_s(lineNo, buff, 500, 10);
#else
				_itow_s(lineNo, buff, 10);
#endif
				rValue += buff;
			} else {
				rValue += "?";
			}
			break;
		case  sdaiINTEGER:
			integer = 0;
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char *) propertyName, sdaiINTEGER, &integer);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiINTEGER, &integer);
			}
#ifdef	WIN64
			_i64tow_s(integer, buff, 500, 10);
#else
			_itow_s(integer, buff, 10);
#endif
			rValue += buff;
			break;
		case  sdaiLOGICAL:
		case  sdaiREAL:
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char*) propertyName, sdaiUNICODE, &buffer);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiUNICODE, &buffer);
			}

			if	(buffer  &&  buffer[0]) {
				rValue += buffer;
			} else {
				rValue += "?";
			}
			break;
		case  sdaiSTRING:
			if	(ifcEntityInstance) {
				sdaiGetAttrBN(ifcEntityInstance, (char*) propertyName, sdaiUNICODE, &buffer);
			} else {
				engiGetAggrElement(ifcAggregate, 0, sdaiUNICODE, &buffer);
			}

			rValue += '\'';
			rValue += buffer;
			rValue += '\'';
			break;
		default:
			rValue += "<NOT IMPLEMENTED>";
			break;
	}

	return	rValue;
}

CString	CreateToolTipText(int_t ifcEntityInstance, int_t entity, int_t * pIndex)
{
	if	(entity) {
		CString	rValue = CreateToolTipText(ifcEntityInstance, engiGetEntityParent(entity), pIndex);

		wchar_t	* entityName = nullptr;
		engiGetEntityName(entity, sdaiUNICODE, (char**) &entityName);
		rValue += entityName;
		rValue += "\n";

		int_t	argCnt = engiGetEntityNoArguments(entity);
		while  ((*pIndex) < argCnt) {
			wchar_t	* propertyName = nullptr;
			engiGetEntityArgumentName(entity, (*pIndex), sdaiUNICODE, (char**) &propertyName);

			int_t	propertyType = 0;
			engiGetEntityArgumentType(entity, (*pIndex), &propertyType);
			rValue += "  ";
			rValue += propertyName;
			rValue += " : ";
			
			rValue += NestedPropertyValue(ifcEntityInstance, propertyName, propertyType, 0);

			rValue += "\n";
			(*pIndex)++;
		}

		return	rValue;
	}

	return	0;
}

CString	CreateToolTipText(int_t ifcEntityInstance)
{
	CString	rValue = 0;
	int_t	index = 0, lineNo, ifcEntity = sdaiGetInstanceType(ifcEntityInstance);
	wchar_t	buff[512];

	lineNo =internalGetP21Line(ifcEntityInstance);
	rValue += "#";
#ifdef	WIN64
	_i64tow_s(lineNo, buff, 500, 10);
#else
	_itow_s(lineNo, buff, 10);
#endif
	rValue += buff;
	rValue += " = ";

	wchar_t	* entityName = nullptr;
	engiGetEntityName(ifcEntity, sdaiUNICODE, (char **) &entityName);

	rValue += entityName;
	rValue += "(...)\n\n";

	rValue += CreateToolTipText(ifcEntityInstance, ifcEntity, &index);

	return	rValue;
}

void CLeftPane::OnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVGETINFOTIP pGetInfoTip = (LPNMTVGETINFOTIP)pNMHDR;

	STRUCT__BASE					* baseItem = (STRUCT__BASE *) GetTreeCtrl().GetItemData(pGetInfoTip->hItem);
	STRUCT__PROPERTY				* propertyItem;
	STRUCT__PROPERTY__SET			* propertySetItem;
	STRUCT__SELECTABLE__TREEITEM	* selectableTreeitem;

	int_t	ifcInstance = 0;
	CString strItemTxt = 0;//m_TreeCtrl.GetItemText(pGetInfoTip->hItem);
	switch  (baseItem->structType) {
		case  STRUCT_TYPE_SELECTABLE_TREEITEM:
			selectableTreeitem = (STRUCT__SELECTABLE__TREEITEM *) baseItem;
			if	(selectableTreeitem->ifcObject  &&  selectableTreeitem->ifcObject->ifcInstance) {
				ifcInstance = selectableTreeitem->ifcObject->ifcInstance;
			} else {
				ifcInstance = selectableTreeitem->ifcInstance;
			}
			break;
		case  STRUCT_TYPE_HEADER_SET:
			break;
		case  STRUCT_TYPE_PROPERTY:
			propertyItem = (STRUCT__PROPERTY *) baseItem;
			ifcInstance = propertyItem->ifcInstance;
			break;
		case  STRUCT_TYPE_PROPERTY_SET:
			propertySetItem = (STRUCT__PROPERTY__SET *) baseItem;
			ifcInstance = propertySetItem->ifcInstance;
			break;
		default:
			ASSERT(false);
			break;
	}

	if	(ifcInstance) {
		strItemTxt = CreateToolTipText(ifcInstance);
	}

	if	(strItemTxt  &&  strItemTxt[0]) {
		strItemTxt = strItemTxt.Left(pGetInfoTip->cchTextMax - 1);
		StrCpyW(pGetInfoTip->pszText, strItemTxt);
	}

	*pResult = 0;
}


void CLeftPane::OnInitialUpdate() 
{
	CTreeView::OnInitialUpdate();
	
	CImageList* pImageList = new CImageList();
	pImageList->Create(16, 16, ILC_COLOR4, 6, 6);

	CBitmap bitmap;

	bitmap.LoadBitmap(IDB_SELECTED_ALL);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_SELECTED_PART);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_SELECTED_NONE);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_PROPERTY_SET);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_PROPERTY);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	bitmap.LoadBitmap(IDB_NONE);
	pImageList->Add(&bitmap, (COLORREF)0x000000);
	bitmap.DeleteObject();

	CTreeCtrl *tst = &GetTreeCtrl();

	glLeftPane = this;

	::SetWindowLong(*tst, GWL_STYLE, TVS_EDITLABELS|TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS|TVS_INFOTIP|::GetWindowLong(*tst, GWL_STYLE));

	GetTreeCtrl().SetImageList(pImageList, TVSIL_NORMAL);
	GetTreeCtrl().DeleteAllItems();

	if	(newFileName) {
		newFileName = false;

		BuildTreeItem(baseTreeItem, false);
	}
}

wchar_t	* getTreeItemName_property(STRUCT__PROPERTY * ifcProperty)
{
	size_t	i = 0;
	if	(ifcProperty->name) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->name, wcslen(ifcProperty->name) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->name);
/*	} else {
		ifcProperty->nameBuffer[i++] = '<';
		ifcProperty->nameBuffer[i++] = 'n';
		ifcProperty->nameBuffer[i++] = 'a';
		ifcProperty->nameBuffer[i++] = 'm';
		ifcProperty->nameBuffer[i++] = 'e';
		ifcProperty->nameBuffer[i++] = '>';	//	*/
	}
	ifcProperty->nameBuffer[i++] = ' ';
	ifcProperty->nameBuffer[i++] = '=';
	ifcProperty->nameBuffer[i++] = ' ';
	if	(ifcProperty->nominalValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->nominalValue, wcslen(ifcProperty->nominalValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->nominalValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->lengthValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->lengthValue, wcslen(ifcProperty->lengthValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->lengthValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->areaValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->areaValue, wcslen(ifcProperty->areaValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->areaValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->volumeValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->volumeValue, wcslen(ifcProperty->volumeValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->volumeValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->countValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->countValue, wcslen(ifcProperty->countValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->countValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->weigthValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->weigthValue, wcslen(ifcProperty->weigthValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->weigthValue);
		ifcProperty->nameBuffer[i++] = ' ';
	} else if  (ifcProperty->timeValue) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->timeValue, wcslen(ifcProperty->timeValue) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->timeValue);
		ifcProperty->nameBuffer[i++] = ' ';
	}
	if	(ifcProperty->unit) {
		memcpy(&ifcProperty->nameBuffer[i], ifcProperty->unit, wcslen(ifcProperty->unit) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcProperty->unit);
		ifcProperty->nameBuffer[i++] = ' ';
	}
	if	(ifcProperty->description) {
		ifcProperty->nameBuffer[i++] = '(';
		if	(ifcProperty->description) {
			ifcProperty->nameBuffer[i++] = '\'';
			memcpy(&ifcProperty->nameBuffer[i], ifcProperty->description, wcslen(ifcProperty->description) * sizeof(wchar_t)/sizeof(char));
			i += wcslen(ifcProperty->description);
			ifcProperty->nameBuffer[i++] = '\'';
		} else {
			ifcProperty->nameBuffer[i++] = '<';
			ifcProperty->nameBuffer[i++] = 'd';
			ifcProperty->nameBuffer[i++] = 'e';
			ifcProperty->nameBuffer[i++] = 's';
			ifcProperty->nameBuffer[i++] = 'c';
			ifcProperty->nameBuffer[i++] = 'r';
			ifcProperty->nameBuffer[i++] = 'i';
			ifcProperty->nameBuffer[i++] = 'p';
			ifcProperty->nameBuffer[i++] = 't';
			ifcProperty->nameBuffer[i++] = 'i';
			ifcProperty->nameBuffer[i++] = 'o';
			ifcProperty->nameBuffer[i++] = 'n';
			ifcProperty->nameBuffer[i++] = '>';
		}
		ifcProperty->nameBuffer[i++] = ')';
	}
	ifcProperty->nameBuffer[i] = 0;			
	return	ifcProperty->nameBuffer;
}

wchar_t	* getTreeItemName_headerSet(STRUCT__HEADER__SET * ifcHeaderSet)
{
	size_t	i = 0;
	if	(ifcHeaderSet->name) {
		memcpy(&ifcHeaderSet->nameBuffer[i], ifcHeaderSet->name, wcslen(ifcHeaderSet->name) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcHeaderSet->name);
	} else {
		ifcHeaderSet->nameBuffer[i++] = '<';
		ifcHeaderSet->nameBuffer[i++] = 'n';
		ifcHeaderSet->nameBuffer[i++] = 'a';
		ifcHeaderSet->nameBuffer[i++] = 'm';
		ifcHeaderSet->nameBuffer[i++] = 'e';
		ifcHeaderSet->nameBuffer[i++] = '>';
	}
	if	(!ifcHeaderSet->child) {
		ifcHeaderSet->nameBuffer[i++] = ' ';
		ifcHeaderSet->nameBuffer[i++] = '=';
		ifcHeaderSet->nameBuffer[i++] = ' ';
		if	(ifcHeaderSet->value) {
			ifcHeaderSet->nameBuffer[i++] = '\'';
			memcpy(&ifcHeaderSet->nameBuffer[i], ifcHeaderSet->value, wcslen(ifcHeaderSet->value) * sizeof(wchar_t)/sizeof(char));
			i += wcslen(ifcHeaderSet->value);
			ifcHeaderSet->nameBuffer[i++] = '\'';
		} else {
			ifcHeaderSet->nameBuffer[i++] = '<';
			ifcHeaderSet->nameBuffer[i++] = 'v';
			ifcHeaderSet->nameBuffer[i++] = 'a';
			ifcHeaderSet->nameBuffer[i++] = 'l';
			ifcHeaderSet->nameBuffer[i++] = 'u';
			ifcHeaderSet->nameBuffer[i++] = 'e';
			ifcHeaderSet->nameBuffer[i++] = '>';
		}
	}
	ifcHeaderSet->nameBuffer[i] = 0;			
	return	ifcHeaderSet->nameBuffer;
}

wchar_t	* getTreeItemName_propertySet(STRUCT__PROPERTY__SET * ifcPropertySet)
{
	size_t	i = 0;
	if	(ifcPropertySet->name) {
		ifcPropertySet->nameBuffer[i++] = '\'';
		memcpy(&ifcPropertySet->nameBuffer[i], ifcPropertySet->name, wcslen(ifcPropertySet->name) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcPropertySet->name);
		ifcPropertySet->nameBuffer[i++] = '\'';
	} else {
		ifcPropertySet->nameBuffer[i++] = '<';
		ifcPropertySet->nameBuffer[i++] = 'n';
		ifcPropertySet->nameBuffer[i++] = 'a';
		ifcPropertySet->nameBuffer[i++] = 'm';
		ifcPropertySet->nameBuffer[i++] = 'e';
		ifcPropertySet->nameBuffer[i++] = '>';
	}
	ifcPropertySet->nameBuffer[i++] = ' ';
	ifcPropertySet->nameBuffer[i++] = '(';
	if	(ifcPropertySet->description) {
		ifcPropertySet->nameBuffer[i++] = '\'';
		memcpy(&ifcPropertySet->nameBuffer[i], ifcPropertySet->description, wcslen(ifcPropertySet->description) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcPropertySet->description);
		ifcPropertySet->nameBuffer[i++] = '\'';
	} else {
		ifcPropertySet->nameBuffer[i++] = '<';
		ifcPropertySet->nameBuffer[i++] = 'd';
		ifcPropertySet->nameBuffer[i++] = 'e';
		ifcPropertySet->nameBuffer[i++] = 's';
		ifcPropertySet->nameBuffer[i++] = 'c';
		ifcPropertySet->nameBuffer[i++] = 'r';
		ifcPropertySet->nameBuffer[i++] = 'i';
		ifcPropertySet->nameBuffer[i++] = 'p';
		ifcPropertySet->nameBuffer[i++] = 't';
		ifcPropertySet->nameBuffer[i++] = 'i';
		ifcPropertySet->nameBuffer[i++] = 'o';
		ifcPropertySet->nameBuffer[i++] = 'n';
		ifcPropertySet->nameBuffer[i++] = '>';
	}
	ifcPropertySet->nameBuffer[i++] = ')';
	ifcPropertySet->nameBuffer[i] = 0;			
	return	ifcPropertySet->nameBuffer;
}

// 这个是节点文字的函数，在这里进行修改
wchar_t	* getTreeItemName_selectableTreeItem(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem)
{
	size_t	i = 0;
	// 增加guid的输出
	wchar_t	* ifcType, * name, * description, *guid;
	if	(selectableTreeitem->ifcObject  &&
		 !equals(selectableTreeitem->ifcType, L"geometry")  &&
		 !equals(selectableTreeitem->ifcType, L"properties")) {
		ifcType = selectableTreeitem->ifcObject->ifcType;
		name = selectableTreeitem->ifcObject->name;
		description = selectableTreeitem->ifcObject->description;
		guid = selectableTreeitem->globalId;
	} else {
		ifcType = selectableTreeitem->ifcType;
		name = selectableTreeitem->name;
		description = selectableTreeitem->description;
		guid = selectableTreeitem->globalId;
	}
	if	(ifcType) {
		memcpy(&selectableTreeitem->nameBuffer[i], ifcType, wcslen(ifcType) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(ifcType);
	}
	// 增加guid的输出
	if(guid)
	{
		selectableTreeitem->nameBuffer[i++] = ' ';
		selectableTreeitem->nameBuffer[i++] = '\'';
		memcpy(&selectableTreeitem->nameBuffer[i], guid, wcslen(guid) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(guid);
		selectableTreeitem->nameBuffer[i++] = '\'';
	}
	if	(name) {
		selectableTreeitem->nameBuffer[i++] = ' ';
		selectableTreeitem->nameBuffer[i++] = '\'';
		memcpy(&selectableTreeitem->nameBuffer[i], name, wcslen(name) * sizeof(wchar_t)/sizeof(char));
		i += wcslen(name);
		selectableTreeitem->nameBuffer[i++] = '\'';
/*	} else {
		selectableTreeitem->nameBuffer[i++] = '<';
		selectableTreeitem->nameBuffer[i++] = 'n';
		selectableTreeitem->nameBuffer[i++] = 'a';
		selectableTreeitem->nameBuffer[i++] = 'm';
		selectableTreeitem->nameBuffer[i++] = 'e';
		selectableTreeitem->nameBuffer[i++] = '>';	//	*/
	}
	if	(description) {
		selectableTreeitem->nameBuffer[i++] = ' ';
		selectableTreeitem->nameBuffer[i++] = '(';
		if	(description) {
			selectableTreeitem->nameBuffer[i++] = '\'';
			memcpy(&selectableTreeitem->nameBuffer[i], description, wcslen(description) * sizeof(wchar_t)/sizeof(char));
			i += wcslen(description);
			selectableTreeitem->nameBuffer[i++] = '\'';
		} else {
			selectableTreeitem->nameBuffer[i++] = '<';
			selectableTreeitem->nameBuffer[i++] = 'd';
			selectableTreeitem->nameBuffer[i++] = 'e';
			selectableTreeitem->nameBuffer[i++] = 's';
			selectableTreeitem->nameBuffer[i++] = 'c';
			selectableTreeitem->nameBuffer[i++] = 'r';
			selectableTreeitem->nameBuffer[i++] = 'i';
			selectableTreeitem->nameBuffer[i++] = 'p';
			selectableTreeitem->nameBuffer[i++] = 't';
			selectableTreeitem->nameBuffer[i++] = 'i';
			selectableTreeitem->nameBuffer[i++] = 'o';
			selectableTreeitem->nameBuffer[i++] = 'n';
			selectableTreeitem->nameBuffer[i++] = '>';
		}
		selectableTreeitem->nameBuffer[i++] = ')';
	}
	selectableTreeitem->nameBuffer[i] = 0;			
	return	selectableTreeitem->nameBuffer;
}

void	CLeftPane::BuildTreeItem(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, bool disableItems)
{
	while  (selectableTreeitem) {
		TV_INSERTSTRUCT		tvstruct;
		if	(selectableTreeitem->parent) {
			tvstruct.hParent = selectableTreeitem->parent->hTreeItem;
		} else {
			tvstruct.hParent = NULL;
		}
		if	(selectableTreeitem->parent == 0) {
			tvstruct.hInsertAfter = TVI_FIRST;
		} else {
			tvstruct.hInsertAfter = TVI_LAST;
		}
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
		if	(selectableTreeitem->child) {
			tvstruct.item.cChildren = 1;
		} else {
			if	(selectableTreeitem->ifcObject  &&  selectableTreeitem->ifcObject->treeItemProperties == selectableTreeitem  &&  (selectableTreeitem->ifcObject->propertySets  ||  selectableTreeitem->ifcObject->propertySetsAvailableButNotLoadedYet)) {
				//
				//	Special case where property sets are available, however this needs to be loaded dynamically for performance reasons.
				//
				tvstruct.item.cChildren = 1;
			} else {
				if	(selectableTreeitem->headerSet) {
					//
					//	Special case of header item, will be loaded dynamically.
					//
					tvstruct.item.cChildren = 1;
				} else {
					tvstruct.item.cChildren = 0;
				}
			}
		}
		selectableTreeitem->select = selectableTreeitem->select;
		tvstruct.item.pszText = getTreeItemName_selectableTreeItem(selectableTreeitem);
		if	(selectableTreeitem->ifcObject  &&  selectableTreeitem->ifcObject->treeItemProperties == selectableTreeitem) {
			tvstruct.item.iImage = ITEM_PROPERTY_SET;
			tvstruct.item.iSelectedImage = ITEM_PROPERTY_SET;
			selectableTreeitem->ignore = true;
		} else {
			if	(disableItems  &&  selectableTreeitem->select == ITEM_CHECKED) {
				selectableTreeitem->select = ITEM_UNCHECKED;
			}
			tvstruct.item.iImage = selectableTreeitem->select;
			tvstruct.item.iSelectedImage = selectableTreeitem->select;
		}
		tvstruct.item.lParam = (LPARAM) selectableTreeitem;

		ASSERT(selectableTreeitem->hTreeItem == 0);
		selectableTreeitem->hTreeItem = GetTreeCtrl().InsertItem(&tvstruct);
		BuildTreeItem(selectableTreeitem->child, disableItems);

/*		if	(selectableTreeitem->ifcItem  &&  selectableTreeitem->ifcItem->hideChildren) {
			if	( (selectableTreeitem->ifcItem)  &&
				  (selectableTreeitem->next)  &&
				  (selectableTreeitem->next->next)  &&
				  (selectableTreeitem->next->next->child)  &&
				  (selectableTreeitem->next->next->child->child)  &&
				  (selectableTreeitem->next->next->child->child->ifcItem) ) {
				selectableTreeitem->ifcItem->materials = selectableTreeitem->next->next->child->child->ifcItem->materials;
			}
			disableItems = true;
//			switch  (select) {
//				case  ITEM_CHECKED:
//					select = ITEM_UNCHECKED;
//					break;
//				default:
//					break;
//			}
		}*/

		selectableTreeitem = selectableTreeitem->next;
	}
}

void	CLeftPane::BuildPropertyTreeItem(STRUCT__PROPERTY__SET * ifcPropertySet)
{
	STRUCT__PROPERTY	* ifcProperty = ifcPropertySet->properties;
	while  (ifcProperty) {
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.hParent = ifcPropertySet->hTreeItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
		tvstruct.item.cChildren = 0;
		tvstruct.item.pszText = getTreeItemName_property(ifcProperty);
		tvstruct.item.iImage = ITEM_PROPERTY;
		tvstruct.item.iSelectedImage = ITEM_PROPERTY;
		tvstruct.item.lParam = (LPARAM) ifcProperty;

		ASSERT(ifcProperty->hTreeItem == 0);
		ifcProperty->hTreeItem = GetTreeCtrl().InsertItem(&tvstruct);

		ifcProperty = ifcProperty->next;
	}
}

HTREEITEM	CLeftPane::SelectTreeItem(STRUCT__IFC__OBJECT * ifcObject, HTREEITEM hItem)
{
	while  (hItem) {
		STRUCT__SELECTABLE__TREEITEM	* selectableTreeitem = (STRUCT__SELECTABLE__TREEITEM *) GetTreeCtrl().GetItemData(hItem);

		if	(selectableTreeitem->structType == STRUCT_TYPE_SELECTABLE_TREEITEM  &&  selectableTreeitem->ifcObject == ifcObject) {
			GetTreeCtrl().SetFocus();
			GetTreeCtrl().SelectItem(hItem);
//			GetTreeCtrl().Select(hItem, TVGN_FIRSTVISIBLE);
			return	hItem;
		}

		HTREEITEM selectedItem = SelectTreeItem(ifcObject, GetTreeCtrl().GetNextItem(hItem, TVGN_CHILD));
		if	(selectedItem) {
			GetTreeCtrl().Expand(hItem, TVE_EXPAND);
			return	selectedItem;
		}
		hItem = GetTreeCtrl().GetNextItem(hItem, TVGN_NEXT);
	}
	return	0;
}

void	CLeftPane::BuildPropertySetTreeItem(STRUCT__SELECTABLE__TREEITEM * selectableTreeItem)
{
	STRUCT__IFC__OBJECT		* ifcObject = selectableTreeItem->ifcObject;
	STRUCT__PROPERTY__SET	* ifcPropertySet = ifcObject->propertySets;
	while  (ifcPropertySet) {
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.hParent = selectableTreeItem->hTreeItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
		if	(ifcPropertySet->properties) {
			tvstruct.item.cChildren = 1;
		} else {
			tvstruct.item.cChildren = 0;
		}
		tvstruct.item.pszText = getTreeItemName_propertySet(ifcPropertySet);
		tvstruct.item.iImage = ITEM_PROPERTY_SET;
		tvstruct.item.iSelectedImage = ITEM_PROPERTY_SET;
		tvstruct.item.lParam = (LPARAM) ifcPropertySet;

		ASSERT(ifcPropertySet->hTreeItem == 0);
		ifcPropertySet->hTreeItem = GetTreeCtrl().InsertItem(&tvstruct);

		ifcPropertySet = ifcPropertySet->next;
	}
}

void	CLeftPane::BuildHeaderSetTreeItem(STRUCT__HEADER__SET * headerSet, HTREEITEM parentHTreeItem)
{
	while  (headerSet) {
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.hParent = parentHTreeItem;
		tvstruct.hInsertAfter = TVI_LAST;
		tvstruct.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
		if	(headerSet->child) {
			tvstruct.item.cChildren = 1;
		} else {
			tvstruct.item.cChildren = 0;
		}
		tvstruct.item.pszText = getTreeItemName_headerSet(headerSet);
		tvstruct.item.iImage = ITEM_PROPERTY_SET;
		tvstruct.item.iSelectedImage = ITEM_PROPERTY_SET;
		tvstruct.item.lParam = (LPARAM) headerSet;

		ASSERT(headerSet->hTreeItem == 0);
		headerSet->hTreeItem = GetTreeCtrl().InsertItem(&tvstruct);

		headerSet = headerSet->next;
	}
}

void	CLeftPane::Update(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, int32_t select) 
{
	ASSERT(select >= 0  &&  select < 100);
	if	(selectableTreeitem) {
		if	(selectableTreeitem->ignore == false  &&  selectableTreeitem->select != select  &&  (selectableTreeitem->select == ITEM_CHECKED  ||  selectableTreeitem->select == ITEM_PARTLY_CHECKED  ||  selectableTreeitem->select == ITEM_UNCHECKED)) {
			ASSERT(selectableTreeitem->select >= 0  &&  selectableTreeitem->select < 100);
			selectableTreeitem->select = select;
			GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);
		}

		int32_t	subSelect = select;
/*		if	(selectableTreeitem->ifcItem  &&  selectableTreeitem->ifcItem->hideChildren) {
			switch  (select) {
				case  ITEM_CHECKED:
					subSelect = ITEM_UNCHECKED;
					break;
				default:
					break;
			}
		}*/
		Update(selectableTreeitem->child, select);
		Update(selectableTreeitem->next, subSelect);
	}
}

void	CLeftPane::UpdateSelect(STRUCT__SELECTABLE__TREEITEM * selectableTreeitem, int_t * pUnchecked, int_t * pChecked)
{
	while  (selectableTreeitem) {
		if	(selectableTreeitem->ignore == false) {
			bool	toAdjust = false;
			int_t	childUnchecked = 0, childChecked = 0;
			UpdateSelect(selectableTreeitem->child, &childUnchecked, &childChecked);
			(* pUnchecked) += childUnchecked;
			(* pChecked) += childChecked;
			switch  (selectableTreeitem->select) {
				case  ITEM_UNCHECKED_TO_ADJUST:
					ASSERT(childUnchecked == 0  &&  childChecked == 0);
					toAdjust = true;
					selectableTreeitem->select = ITEM_UNCHECKED;
					(* pUnchecked)++;
					break;
				case  ITEM_CHECKED_TO_ADJUST:
					ASSERT(childUnchecked == 0  &&  childChecked == 0);
					toAdjust = true;
					selectableTreeitem->select = ITEM_CHECKED;
					(* pChecked)++;
					break;
				case  ITEM_UNCHECKED:
					if	(childChecked) {
						toAdjust = true;
					} else {
						(* pUnchecked)++;
					}
					break;
				case  ITEM_PARTLY_CHECKED:
					if	(childUnchecked == 0  ||  childChecked == 0) {
						toAdjust = true;
					}
					break;
				case  ITEM_CHECKED:
					if	(childUnchecked) {
						toAdjust = true;
					} else {
						(* pChecked)++;
					}
					break;
				case  ITEM_NONE:
				case  ITEM_PROPERTY:
				case  ITEM_PROPERTY_SET:
					break;
				default:
					ASSERT(false);
					break;
			}

			if	(toAdjust) {
				if	(childUnchecked) {
					if	(childChecked) {
						selectableTreeitem->select = ITEM_PARTLY_CHECKED;
					} else {
						selectableTreeitem->select = ITEM_UNCHECKED;
					}
				} else {
					if	(childChecked) {
						selectableTreeitem->select = ITEM_CHECKED;
					}
				}

				GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);
			}
		}
		
		selectableTreeitem = selectableTreeitem->next;
	}
}

void	CLeftPane::OnRClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	pNMHDR = pNMHDR;
	pResult = pResult;

	DWORD	posa = GetMessagePos();
	CPoint	pta(LOWORD(posa), HIWORD(posa));

	HMENU	hMenu = ::CreatePopupMenu();
	int_t	i = 1;

	STRUCT__IFC__OBJECT	* ifcObject = ifcObjectsLinkedList;
	while  (ifcObject) {
		bool	visualized = false;
		int_t	realGeometry = 0,
				currentIfcEntity = ifcObject->ifcEntity;
		while  (ifcObject  &&  ifcObject->ifcEntity == currentIfcEntity) {
			if	(ifcObject->treeItemGeometry  &&  ifcObject->treeItemGeometry->select == ITEM_CHECKED) {
				visualized = true;
			}
			if	(ifcObject->treeItemGeometry  &&  (ifcObject->noPrimitivesForFaces  ||  ifcObject->noPrimitivesForWireFrame) ) {
				realGeometry++;
			}
			ifcObject = ifcObject->next;
		}
		int32_t	flags = 0;
		if	(visualized) {
			flags |= MF_CHECKED;
		}
		if	(!realGeometry) {
			flags |= MF_DISABLED;
		}
		wchar_t	* entityName = 0;
		engiGetEntityName(currentIfcEntity, sdaiUNICODE, (char**) &entityName);
		::AppendMenu(hMenu, flags, i++, entityName);
	}
	int_t sel = ::TrackPopupMenuEx(hMenu, 
		TPM_CENTERALIGN | TPM_RETURNCMD,
		pta.x,//pt.x + r.right,
		pta.y,//pt.y + r.top,
		GetTreeCtrl(),
		NULL);
	::DestroyMenu(hMenu);
	if	(sel > 0) {
		ifcObject = ifcObjectsLinkedList;
		while  (ifcObject) {
			int_t	currentIfcEntity = ifcObject->ifcEntity;
			sel--;
			if	(sel == 0) {
				STRUCT__IFC__OBJECT	* ifcStartingObject = ifcObject;
				bool	visualized = false;
				while  (ifcObject  &&  ifcObject->ifcEntity == currentIfcEntity) {
					if	(ifcObject->treeItemGeometry  &&  ifcObject->treeItemGeometry->select == ITEM_CHECKED) {
						visualized = true;
					}
					ifcObject = ifcObject->next;
				}
				ifcObject = ifcStartingObject;
				while  (ifcObject  &&  ifcObject->ifcEntity == currentIfcEntity) {
					if	(ifcObject->treeItemGeometry) {
						if	(visualized) {
							if	(ifcObject->treeItemGeometry->select == ITEM_CHECKED) {
								ifcObject->treeItemGeometry->select = ITEM_UNCHECKED_TO_ADJUST;
							}
						} else {
							if	(ifcObject->treeItemGeometry->select == ITEM_UNCHECKED) {
								ifcObject->treeItemGeometry->select = ITEM_CHECKED_TO_ADJUST;
							}
						}
					}
					ifcObject = ifcObject->next;
				}
			} else {
				while  (ifcObject  &&  ifcObject->ifcEntity == currentIfcEntity) {
					ifcObject = ifcObject->next;
				}
			}
		}

		//
		//	Update Left Pane
		//
		int_t	unchecked = 0, checked = 0;
		UpdateSelect(baseTreeItem, &unchecked, &checked);

		//
		//	Update Right Pane
		//
		this->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
	}
}

void	CLeftPane::OnClick(NMHDR* pNMHDR, LRESULT* pResult) 
{
	pNMHDR = pNMHDR;

	STRUCT__SELECTABLE__TREEITEM		* selectableTreeitem;

	DWORD		pos = GetMessagePos();
	CPoint		pt(LOWORD(pos), HIWORD(pos));
	GetTreeCtrl().ScreenToClient(&pt);

	// Get indexes of the first and last visible items in listview
	// control.
	HTREEITEM	hItem = GetTreeCtrl().GetFirstVisibleItem();
	bool		updated = false;

	while  (hItem) {
		CRect r;
		GetTreeCtrl().GetItemRect(hItem, &r, true);
		r.right = r.left-4;
		r.left = r.left-18;

		if (r.PtInRect(pt))
		{
			bool	quit = false;

			selectableTreeitem = (STRUCT__SELECTABLE__TREEITEM *) GetTreeCtrl().GetItemData(hItem);

			if	(selectableTreeitem->structType == STRUCT_TYPE_SELECTABLE_TREEITEM) {
				switch  (selectableTreeitem->select) {
					case  ITEM_CHECKED:
						selectableTreeitem->select = ITEM_UNCHECKED;
						break;
					case  ITEM_PARTLY_CHECKED:
						selectableTreeitem->select = ITEM_UNCHECKED;
						break;
					case  ITEM_UNCHECKED:
						selectableTreeitem->select = ITEM_CHECKED;
						break;
					case  ITEM_PROPERTY_SET:
					case  ITEM_PROPERTY:
					case  ITEM_NONE:
						quit = true;
						break;
					default:
						ASSERT(false);
						break;
				}

				if	(!quit) {
					GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);
					updated = true;

					//
					//	Update Children
					//
					Update(selectableTreeitem->child, selectableTreeitem->select);

					//
					//	Update Parents
					//
					while  (selectableTreeitem->parent) {
						selectableTreeitem = selectableTreeitem->parent;
						STRUCT__SELECTABLE__TREEITEM	* child = selectableTreeitem->child;
						int_t	checked = false, unchecked = false;
						while  (child) {
							if	(child->ignore == false) {
								switch  (child->select) {
									case  ITEM_CHECKED:
										checked = true;
										break;
									case  ITEM_PARTLY_CHECKED:
										checked = true;
										unchecked = true;
										break;
									case  ITEM_UNCHECKED:
										unchecked = true;
										break;
									case  ITEM_PROPERTY_SET:
									case  ITEM_PROPERTY:
									case  ITEM_NONE:
										break;
									default:
										ASSERT(false);
										break;
								}
							}
							child = child->next;
						}
						if	(checked) {
							if	(unchecked) {
								selectableTreeitem->select = ITEM_PARTLY_CHECKED;
							} else {
								selectableTreeitem->select = ITEM_CHECKED;
							}
						} else {
							selectableTreeitem->select = ITEM_UNCHECKED;
						}
						GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);
					}
				}
			}
		}

		hItem = GetTreeCtrl().GetNextVisibleItem(hItem);
	}

	//
	//	Update right window
	//

	if	(updated) {
		this->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
	}

	*pResult = 0;
}

void	CLeftPane::OnSelectionChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	pResult = pResult;

	NMTREEVIEW* pnmtv = (NMTREEVIEW*) pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	STRUCT__SELECTABLE__TREEITEM	* selectableTreeitem = (STRUCT__SELECTABLE__TREEITEM *) GetTreeCtrl().GetItemData(hItem);
	if	(selectableTreeitem->structType == STRUCT_TYPE_SELECTABLE_TREEITEM) {
		highLightedIfcObject = selectableTreeitem->ifcObject;
		this->GetWindow(GW_HWNDNEXT)->SendMessage(IDS_UPDATE_RIGHT_PANE, 0, 0);
	}
}

void	CLeftPane::OnBeforeExpand(NMHDR* pNMHDR, LRESULT* pResult)
{
	pResult = pResult;

	NMTREEVIEW* pnmtv = (NMTREEVIEW*) pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	STRUCT__HEADER__SET				* ifcHeaderSet;
	STRUCT__PROPERTY__SET			* ifcPropertySet;
	STRUCT__SELECTABLE__TREEITEM	* selectableTreeitem;
	STRUCT__BASE					* base = (STRUCT__BASE *) GetTreeCtrl().GetItemData(hItem);

	switch	(base->structType) {
		case  STRUCT_TYPE_SELECTABLE_TREEITEM:
			selectableTreeitem = (STRUCT__SELECTABLE__TREEITEM *) base;
			if	(selectableTreeitem->ifcObject  &&  selectableTreeitem->ifcObject->propertySetsAvailableButNotLoadedYet) {
				ASSERT(selectableTreeitem->ifcObject->propertySets == 0);
				CreateIfcInstanceProperties(globalIfcModel, &selectableTreeitem->ifcObject->propertySets, selectableTreeitem->ifcObject->ifcInstance, selectableTreeitem->ifcObject->units);
				ASSERT(selectableTreeitem->ifcObject->propertySets != 0);
				selectableTreeitem->ifcObject->propertySetsAvailableButNotLoadedYet = false;
			}
			if	(selectableTreeitem->ifcObject  &&  selectableTreeitem->ifcObject->propertySets  &&  selectableTreeitem->ifcObject->propertySets->hTreeItem == 0) {
				BuildPropertySetTreeItem(selectableTreeitem);
			}
			if	(selectableTreeitem->headerSet) {
				BuildHeaderSetTreeItem(selectableTreeitem->headerSet->child, selectableTreeitem->hTreeItem);
			}
			break;
		case  STRUCT_TYPE_PROPERTY_SET:
			ifcPropertySet = (STRUCT__PROPERTY__SET *) base;
			if	(ifcPropertySet->properties  &&  ifcPropertySet->properties->hTreeItem == 0) {
				BuildPropertyTreeItem(ifcPropertySet);
			}
			break;
		case  STRUCT_TYPE_HEADER_SET:
			ifcHeaderSet = (STRUCT__HEADER__SET *) base;
			if	(ifcHeaderSet->child  &&  ifcHeaderSet->child->hTreeItem == 0) {
				BuildHeaderSetTreeItem(ifcHeaderSet->child, ifcHeaderSet->hTreeItem);
			}
			break;
		default:
			ASSERT(false);
			break;
	}
}

LRESULT CLeftPane::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	STRUCT__SELECTABLE__TREEITEM	* selectableTreeitem;
	HTREEITEM						hItem;
	switch  (message)
	{
		case IDS_DISABLE_ITEM:
			if	(highLightedIfcObject  &&  highLightedIfcObject->treeItemGeometry) {
				selectableTreeitem = highLightedIfcObject->treeItemGeometry;

				selectableTreeitem->select = ITEM_UNCHECKED;
				GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);

				//
				//	Update Children
				//
				Update(selectableTreeitem->child, selectableTreeitem->select);

				//
				//	Update Parents
				//
				while  (selectableTreeitem->parent) {
					selectableTreeitem = selectableTreeitem->parent;
					STRUCT__SELECTABLE__TREEITEM	* child = selectableTreeitem->child;
					int_t	checked = false, unchecked = false;
					while  (child) {
						if	(child->ignore == false) {
							switch  (child->select) {
								case  ITEM_CHECKED:
									checked = true;
									break;
								case  ITEM_PARTLY_CHECKED:
									checked = true;
									unchecked = true;
									break;
								case  ITEM_UNCHECKED:
									unchecked = true;
									break;
								case  ITEM_PROPERTY_SET:
								case  ITEM_PROPERTY:
								case  ITEM_NONE:
									break;
								default:
									ASSERT(false);
									break;
							}
						}
						child = child->next;
					}
					if	(checked) {
						if	(unchecked) {
							selectableTreeitem->select = ITEM_PARTLY_CHECKED;
						} else {
							selectableTreeitem->select = ITEM_CHECKED;
						}
					} else {
						selectableTreeitem->select = ITEM_UNCHECKED;
					}
					GetTreeCtrl().SetItemImage(selectableTreeitem->hTreeItem, selectableTreeitem->select, selectableTreeitem->select);
				}
			}
			break;
		case IDS_SELECT_ITEM:
			hItem = SelectTreeItem(highLightedIfcObject, GetTreeCtrl().GetRootItem());
			GetTreeCtrl().Select(hItem, TVGN_FIRSTVISIBLE);
			break;
		default:
			break;
	}

	return	CTreeView::WindowProc(message, wParam, lParam);
}

// CLeftPane diagnostics

#ifdef _DEBUG
void CLeftPane::AssertValid() const
{
	CTreeView::AssertValid();
}

#ifndef _WIN32_WCE
void CLeftPane::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}
#endif
#endif //_DEBUG
