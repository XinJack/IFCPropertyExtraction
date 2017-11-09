#include "stdafx.h"
#include "IFCEngineInteract.h"
#include "material.h"
#include "headerInfo.h"
#include "unit.h"

#include "ProgressWnd.h"


#include <unordered_map>


std::list<D3DMATERIAL9*> mtrls;
extern	STRUCT_MATERIAL	* firstMaterial;
extern	std::list<D3DMATERIAL9*> mtrls;

wchar_t	ifcFileName[512], * ifcSchemaName_IFC2x3 = 0, * ifcSchemaName_IFC4 = 0, * xmlSettings_IFC2x3 = 0, * xmlSettings_IFC4 = 0;

STRUCT__SELECTABLE__TREEITEM	* baseTreeItem = nullptr;
STRUCT__IFC__OBJECT				* ifcObjectsLinkedList = nullptr;

int_t	globalIfcModel = 0;
bool	firstItemWithGeometryPassed;

std::unordered_map<int_t, STRUCT__IFC__OBJECT *> allIfcObjects;

STRUCT__SELECTABLE__TREEITEM	** CreateTreeItem_ifcObjectDecomposedBy(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM * parent, STRUCT__SELECTABLE__TREEITEM ** ppRelation, int_t ifcObjectInstance, STRUCT__SIUNIT * units);
STRUCT__SELECTABLE__TREEITEM	** CreateTreeItem_ifcObjectContains(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM * parent, STRUCT__SELECTABLE__TREEITEM ** ppRelation, int_t ifcObjectInstance, STRUCT__SIUNIT * units);


STRUCT__SELECTABLE__TREEITEM	** CreateTreeItem_ifcObject(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM * parent, STRUCT__SELECTABLE__TREEITEM ** ppRelation, int_t ifcObjectInstance, STRUCT__SIUNIT * units)
{
	STRUCT__IFC__OBJECT	* ifcObject = allIfcObjects[ifcObjectInstance];

	wchar_t	* globalId = nullptr, * name = nullptr, * description = nullptr;
	sdaiGetAttrBN(ifcObjectInstance, (char*) L"GlobalId", sdaiUNICODE, &globalId);
	sdaiGetAttrBN(ifcObjectInstance, (char*) L"Name", sdaiUNICODE, &name);
	sdaiGetAttrBN(ifcObjectInstance, (char*) L"Description", sdaiUNICODE, &description);

	if	(ifcObject == 0) {
		(*ppRelation) = CreateTreeItem(parent, ifcObjectInstance, L"???", globalId, name, description);
	} else {
		(*ppRelation) = CreateTreeItem(parent, ifcObjectInstance, ifcObject->ifcType, globalId, name, description);
	}
	STRUCT__SELECTABLE__TREEITEM	** ppChild = &(*ppRelation)->child;

	(*ppChild) = CreateTreeItem((*ppRelation), 0, L"geometry", 0, 0, 0);

	(*ppChild)->ifcObject = ifcObject;
	if	(ifcObject) {
		ifcObject->treeItemGeometry = (*ppChild);
	}

	ppChild = &(* ppChild)->next;

	(* ppChild) = CreateTreeItem((*ppRelation), 0, L"properties", 0, 0, 0);

	(*ppChild)->ifcObject = ifcObject;
	if	(ifcObject) {
		ifcObject->treeItemProperties = (*ppChild);
		ifcObject->units = units;
		if	(HasChildrenIfcInstanceProperties(ifcModel, /*&ifcObject->propertySets,*/ ifcObjectInstance/*, units*/)) {
			ifcObject->propertySetsAvailableButNotLoadedYet = true;
		} else {
			ASSERT(ifcObject->propertySetsAvailableButNotLoadedYet == false);
		}
		ASSERT(ifcObject->propertySets == 0);
	}

	ppChild = &(* ppChild)->next;

	CreateTreeItem_ifcObjectDecomposedBy(ifcModel, (*ppRelation), ppChild, ifcObjectInstance, units);
	if	((*ppChild)) {
		ppChild = &(*ppChild)->next;
	}
	ppChild = CreateTreeItem_ifcObjectContains(ifcModel, (*ppRelation), ppChild, ifcObjectInstance, units);

	ppRelation = &(* ppRelation)->next;

	return	ppRelation;
}

STRUCT__SELECTABLE__TREEITEM	** CreateTreeItem_ifcObjectDecomposedBy(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM * parent, STRUCT__SELECTABLE__TREEITEM ** ppRelation, int_t ifcObjectInstance, STRUCT__SIUNIT * units)
{
	STRUCT__SELECTABLE__TREEITEM	** ppChild = nullptr;

	int_t	* ifcRelDecomposesInstances = nullptr, ifcRelDecomposesInstancesCnt;

	sdaiGetAttrBN(ifcObjectInstance, (char*) L"IsDecomposedBy", sdaiAGGR, &ifcRelDecomposesInstances);

	if	(ifcRelDecomposesInstances) {
		ifcRelDecomposesInstancesCnt = sdaiGetMemberCount(ifcRelDecomposesInstances);
		for  (int_t j = 0; j < ifcRelDecomposesInstancesCnt; ++j) {
			int_t ifcRelDecomposesInstance = 0,

			ifcRelAggregates_TYPE = sdaiGetEntity(ifcModel, (char*) L"IFCRELAGGREGATES");

			engiGetAggrElement(ifcRelDecomposesInstances, j, sdaiINSTANCE, &ifcRelDecomposesInstance);
			if	(sdaiGetInstanceType(ifcRelDecomposesInstance) == ifcRelAggregates_TYPE) {
				int_t	* ifcObjectInstances = nullptr, ifcObjectInstancesCnt;

				sdaiGetAttrBN(ifcRelDecomposesInstance, (char*) L"RelatedObjects", sdaiAGGR, &ifcObjectInstances);

				if	(ppChild == 0) {

					(* ppRelation) = CreateTreeItem(parent, 0, L"decomposition", 0, 0, 0);

					ppChild = &(* ppRelation)->child;
				}

				ifcObjectInstancesCnt = sdaiGetMemberCount(ifcObjectInstances);
				for  (int_t k = 0; k < ifcObjectInstancesCnt; ++k) {
					ifcObjectInstance = 0;
					engiGetAggrElement(ifcObjectInstances, k, sdaiINSTANCE, &ifcObjectInstance);

					CreateTreeItem_ifcObject(ifcModel, (*ppRelation), ppChild, ifcObjectInstance, units);

					ppChild = &(* ppChild)->next;
				}
			} else {
				ASSERT(false);
			}
		}
	}

	return	ppChild;
}

STRUCT__SELECTABLE__TREEITEM	** CreateTreeItem_ifcObjectContains(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM * parent, STRUCT__SELECTABLE__TREEITEM ** ppRelation, int_t ifcObjectInstance, STRUCT__SIUNIT * units)
{
	STRUCT__SELECTABLE__TREEITEM	** ppChild = nullptr;

	int_t	* ifcRelContainedInSpatialStructureInstances = nullptr, ifcRelContainedInSpatialStructureInstancesCnt;

	sdaiGetAttrBN(ifcObjectInstance, (char*) L"ContainsElements", sdaiAGGR, &ifcRelContainedInSpatialStructureInstances);

	if	(ifcRelContainedInSpatialStructureInstances) {
		ifcRelContainedInSpatialStructureInstancesCnt = sdaiGetMemberCount(ifcRelContainedInSpatialStructureInstances);
		if	(ifcRelContainedInSpatialStructureInstancesCnt) {
			for  (int_t i = 0; i < ifcRelContainedInSpatialStructureInstancesCnt; ++i) {
				int_t	ifcRelContainedInSpatialStructureInstance = 0,
						ifcRelContainedInSpatialStructure_TYPE = sdaiGetEntity(ifcModel, (char*) L"IFCRELCONTAINEDINSPATIALSTRUCTURE");

				engiGetAggrElement(ifcRelContainedInSpatialStructureInstances, i, sdaiINSTANCE, &ifcRelContainedInSpatialStructureInstance);
				if	(sdaiGetInstanceType(ifcRelContainedInSpatialStructureInstance) == ifcRelContainedInSpatialStructure_TYPE) {
					int_t	* ifcObjectInstances = nullptr, ifcObjectInstancesCnt;

					sdaiGetAttrBN(ifcRelContainedInSpatialStructureInstance, (char*) L"RelatedElements", sdaiAGGR, &ifcObjectInstances);

					if	(ppChild == 0) {
						(* ppRelation) = CreateTreeItem(parent, 0, L"contains", 0, 0, 0);

						ppChild = &(* ppRelation)->child;
					}

					ifcObjectInstancesCnt = sdaiGetMemberCount(ifcObjectInstances);
					for  (int_t k = 0; k < ifcObjectInstancesCnt; ++k) {
						ifcObjectInstance = 0;
						engiGetAggrElement(ifcObjectInstances, k, sdaiINSTANCE, &ifcObjectInstance);

						CreateTreeItem_ifcObject(ifcModel, (*ppRelation), ppChild, ifcObjectInstance, units);

						ppChild = &(* ppChild)->next;
					}
				} else {
					ASSERT(false);
				}
			}
		}
	}

	return	ppChild;
}

void	CreateTreeItem_ifcProject(int_t ifcModel, STRUCT__SELECTABLE__TREEITEM ** ppParent)
{
	int_t	i, * ifcProjectInstances, noIfcProjectInstances;
	
	ifcProjectInstances = sdaiGetEntityExtentBN(ifcModel, (char*) L"IFCPROJECT");

	noIfcProjectInstances = sdaiGetMemberCount(ifcProjectInstances);
	for (i = 0; i < noIfcProjectInstances; ++i) {
		int_t	ifcProjectInstance = 0;
		engiGetAggrElement(ifcProjectInstances, i, sdaiINSTANCE, &ifcProjectInstance);

		STRUCT__SIUNIT	* units = GetUnits(ifcModel, ifcProjectInstance);

		CreateTreeItem_ifcObject(ifcModel, 0, ppParent, ifcProjectInstance, units);
	}
}

void	CreateTreeItem_nonReferencedIfcItems()
{
	int_t	nonReferencedItemCnt = 0;
	STRUCT__IFC__OBJECT	* ifcObject = ifcObjectsLinkedList;
	while  (ifcObject) {
		ASSERT(ifcObject->ifcInstance);
		if	(ifcObject->treeItemGeometry == nullptr) {
			nonReferencedItemCnt++;
		}
		ifcObject = ifcObject->next;
	}

	if	(nonReferencedItemCnt) {
		wchar_t	buffer[100];
		_itow_s((int32_t) nonReferencedItemCnt, buffer, 100, 10);
		STRUCT__SELECTABLE__TREEITEM	* parentTreeItem = CreateTreeItem(0, 0, L"in IFC file", 0, L"not referenced in structure", buffer),
										** ppChildTreeItem = &parentTreeItem->child;

		parentTreeItem->next = baseTreeItem;
		baseTreeItem = parentTreeItem;

		ifcObject = ifcObjectsLinkedList;
		if	(ifcObject) {
			while  (ifcObject) {
				if	(ifcObject->treeItemGeometry == nullptr) {
					ASSERT(ifcObject->treeItemProperties == nullptr);
					int	items = 0;
					STRUCT__IFC__OBJECT	* ifcObjectSubLoop = ifcObject;
					while  (ifcObjectSubLoop  &&  ifcObjectSubLoop->ifcEntity == ifcObject->ifcEntity) {
						if	(ifcObjectSubLoop->treeItemGeometry == nullptr) {
							items++;
						}
						ifcObjectSubLoop = ifcObjectSubLoop->next;
					}
					wchar_t	*entityName = 0, buffer[100];
					engiGetEntityName(ifcObject->ifcEntity, sdaiUNICODE, (char**) &entityName);

					_itow_s((int32_t) items, buffer, 100, 10);
					(* ppChildTreeItem) = CreateTreeItem(parentTreeItem, 0, L"Set", 0, entityName, buffer);
//
//					STRUCT__SELECTABLE__TREEITEM	* 
//					(* ppChildTreeItem)->ifcItem = ifcItem;
//					ifcItem->treeItemGeometry = (* ppChildTreeItem);
	///////				ASSERT(false);	//	did not implement really adding the ifcItems
				}
				ifcObject = ifcObject->next;
			}
		}
	}
}

STRUCT__IFC__OBJECT	** queryIfcObjects(int_t ifcModel, STRUCT__IFC__OBJECT ** firstFreeIfcObject, wchar_t * ObjectName, bool hide)
{
	int_t	i, * ifcObjectInstances, noIfcObjectInstances;

	ifcObjectInstances = sdaiGetEntityExtentBN(ifcModel, (char*) ObjectName);

	noIfcObjectInstances = sdaiGetMemberCount(ifcObjectInstances);
	if	(noIfcObjectInstances) {
		int_t	ifcEntity = sdaiGetEntity(ifcModel, (char*) ObjectName);
		for (i = 0; i < noIfcObjectInstances; ++i) {
			wchar_t	* globalId = nullptr, * name = nullptr, * description = nullptr;
			int_t	ifcObjectInstance = 0;
			engiGetAggrElement(ifcObjectInstances, i, sdaiINSTANCE, &ifcObjectInstance);

			sdaiGetAttrBN(ifcObjectInstance, (char*) L"GlobalId", sdaiUNICODE, &globalId);
			sdaiGetAttrBN(ifcObjectInstance, (char*) L"Name", sdaiUNICODE, &name);
			sdaiGetAttrBN(ifcObjectInstance, (char*) L"Description", sdaiUNICODE, &description);

			STRUCT__IFC__OBJECT	* ifcObject = CreateIfcObject(ifcEntity, ifcObjectInstance, ObjectName, hide, globalId, name, description);
			(*firstFreeIfcObject) = ifcObject;
			firstFreeIfcObject = &ifcObject->next;

			allIfcObjects[ifcObjectInstance] = ifcObject;
		}
	}

	return	firstFreeIfcObject;
}

double	fabs__(double x)
{
	if	(x < 0) {
		return	-x;
	} else {
		return	x;
	}
}

void	GenerateWireFrameGeometry(int_t ifcModel, STRUCT__IFC__OBJECT * ifcObject)
{
	if	(ifcObject  &&  ifcObject->ifcInstance) {
		int_t	noVertices = 0, noIndices = 0;
		initializeModellingInstance(ifcModel, &noVertices, &noIndices, 0, ifcObject->ifcInstance);

		if	(noVertices  &&  noIndices) {
			ifcObject->noVertices = noVertices;
			ifcObject->vertices = new float[noVertices * 6];    //    <x, y, z>
			int32_t	* indices = new int32_t[noIndices];
			finalizeModelling(ifcModel, (float *) ifcObject->vertices, (int_t *) indices, 0);

			int64_t	owlModel = 0, owlInstance = 0;
			if	(firstItemWithGeometryPassed == false) {
				owlGetModel(ifcModel, &owlModel);
				owlGetInstance(ifcModel, ifcObject->ifcInstance, &owlInstance);
				ASSERT(owlModel  &&  owlInstance);

				double	transformationMatrix[12], minVector[3], maxVector[3];
				SetBoundingBoxReference(owlInstance, transformationMatrix, minVector, maxVector);

				if	( (-1000000 > transformationMatrix[9]   ||  transformationMatrix[9]  > 1000000)  ||
				      (-1000000 > transformationMatrix[10]  ||  transformationMatrix[10] > 1000000)  ||
				      (-1000000 > transformationMatrix[11]  ||  transformationMatrix[11] > 1000000) ) {
					setVertexOffset(ifcModel, -transformationMatrix[9], -transformationMatrix[10], -transformationMatrix[11]);
					int_t	i = 0;
					while  (i < noVertices) {
						ifcObject->vertices[6*i + 0] = ifcObject->vertices[6*i + 0] - (float)transformationMatrix[9];
						ifcObject->vertices[6*i + 1] = ifcObject->vertices[6*i + 1] - (float)transformationMatrix[10];
						ifcObject->vertices[6*i + 2] = ifcObject->vertices[6*i + 2] - (float)transformationMatrix[11];
						i++;
					}
				}
			}

			firstItemWithGeometryPassed = true;

			ifcObject->noPrimitivesForWireFrame = 0;
			ASSERT(ifcObject->indicesForLinesWireFrame == 0);
			int32_t	* indicesForLinesWireFrame = new int32_t[2*noIndices];

			ifcObject->noVertices = noVertices;
			ASSERT(ifcObject->indicesForFaces == 0);
			int32_t	* indicesForFaces = new int32_t[noIndices];

			int_t	faceCnt = getConceptualFaceCnt(ifcObject->ifcInstance);
			for  (int_t j = 0; j < faceCnt; j++) {
				int_t	startIndexTriangles = 0, noIndicesTrangles = 0, startIndexFacesPolygons = 0, noIndicesFacesPolygons = 0;
				getConceptualFaceEx(ifcObject->ifcInstance, j, &startIndexTriangles, &noIndicesTrangles, 0, 0, 0, 0, &startIndexFacesPolygons, &noIndicesFacesPolygons, 0, 0);

				int_t	i = 0;
				while  (i < noIndicesTrangles) {
					indicesForFaces[ifcObject->noPrimitivesForFaces * 3 + i] = indices[startIndexTriangles + i];
					i++;
				}
				ifcObject->noPrimitivesForFaces += noIndicesTrangles/3;

				i = 0;
				int32_t	lastItem = -1;
				while  (i < noIndicesFacesPolygons) {
					if	(lastItem >= 0  &&  indices[startIndexFacesPolygons+i] >= 0) {
						indicesForLinesWireFrame[2*ifcObject->noPrimitivesForWireFrame + 0] = lastItem;
						indicesForLinesWireFrame[2*ifcObject->noPrimitivesForWireFrame + 1] = indices[startIndexFacesPolygons+i];
						ifcObject->noPrimitivesForWireFrame++;
					}
					lastItem = indices[startIndexFacesPolygons+i];
					i++;
				}


			}

if	((equals(ifcObject->ifcType, L"IfcWall")  ||  equals(ifcObject->ifcType, L"IfcSite")  ||  equals(ifcObject->ifcType, L"IfcRoof"))  &&  noVertices < 2000) {
int32_t	* map = new int32_t[noVertices], i = 0;
while  (i < noVertices) {
	map[i] = -1;
	i++;
}
i = 0;
while  (i < noVertices) {
	int_t	j = i + 1;
	if	(map[i] == -1) {
		while  (j < noVertices) {
			if	(map[j] == -1) {
				if	( (ifcObject->vertices[6*i + 0] == ifcObject->vertices[6*j + 0])  &&
					  (ifcObject->vertices[6*i + 1] == ifcObject->vertices[6*j + 1])  &&
					  (ifcObject->vertices[6*i + 2] == ifcObject->vertices[6*j + 2])  &&
					  (fabs__(ifcObject->vertices[6*i + 3] - ifcObject->vertices[6*j + 3]) < 0.00001)  &&
					  (fabs__(ifcObject->vertices[6*i + 4] - ifcObject->vertices[6*j + 4]) < 0.00001)  &&
					  (fabs__(ifcObject->vertices[6*i + 5] - ifcObject->vertices[6*j + 5]) < 0.00001) ) {
					map[j] = i;
				}
			}
			j++;
		}
	}
	i++;
}

i = 0;
while  (i < 2 * ifcObject->noPrimitivesForWireFrame) {
	if	(map[indicesForLinesWireFrame[i]] != -1) {
		indicesForLinesWireFrame[i] = map[indicesForLinesWireFrame[i]];
	}
	i++;
}

delete[] map;
map = 0;

i = 0;
while  (i < ifcObject->noPrimitivesForWireFrame) {
	int_t	j = i + 1;
	while  (j < ifcObject->noPrimitivesForWireFrame) {
		if	( (indicesForLinesWireFrame[2*i + 0] == indicesForLinesWireFrame[2*j + 1])  &&
			  (indicesForLinesWireFrame[2*i + 1] == indicesForLinesWireFrame[2*j + 0]) ) {
			indicesForLinesWireFrame[2*i + 0] = 0;
			indicesForLinesWireFrame[2*i + 1] = 0;
			indicesForLinesWireFrame[2*j + 0] = 0;
			indicesForLinesWireFrame[2*j + 1] = 0;
		}
		j++;
	}
	i++;
}
}

			ifcObject->indicesForFaces = new int32_t[3 * ifcObject->noPrimitivesForFaces];
			ifcObject->indicesForLinesWireFrame = new int32_t[2 * ifcObject->noPrimitivesForWireFrame];
			
			memcpy(ifcObject->indicesForFaces, indicesForFaces, 3 * ifcObject->noPrimitivesForFaces * sizeof(int32_t));
			memcpy(ifcObject->indicesForLinesWireFrame, indicesForLinesWireFrame, 2 * ifcObject->noPrimitivesForWireFrame * sizeof(int32_t));

			delete[]  indicesForLinesWireFrame;
			delete[]  indicesForFaces;
			delete[]  indices;

//			owlGetModel(ifcModel, &owlModel);
//			owlGetInstance(ifcModel, ifcObject->ifcInstance, &owlInstance);
//			ASSERT(owlModel  &&  owlInstance);

			ifcObject->materials = ifcObjectMaterial(ifcModel, ifcObject->ifcInstance);
		} else {
			ASSERT(ifcObject->noVertices == 0);
		}
	}
}

void	GenerateGeometryNestedCall(int_t ifcModel, CProgressWnd * progressWnd, int steps)
{
	int	objectCnt = 0, stepSize;
	STRUCT__IFC__OBJECT	* ifcObject = ifcObjectsLinkedList;
	while  (ifcObject) {
		objectCnt++;
		ifcObject = ifcObject->next;
	}

	stepSize = objectCnt / steps;
	objectCnt = 0;

	ifcObject = ifcObjectsLinkedList;
	while  (ifcObject) {
		if	(objectCnt > stepSize) {
progressWnd->StepIt();
progressWnd->PeekAndPump();
			objectCnt = 0;
		} else {
			objectCnt++;
		}
		//
		//	Get Geometry
		//
		int_t	setting = 0, mask = 0;
		mask += flagbit2;        //    PRECISION (32/64 bit)
		mask += flagbit3;        //	   INDEX ARRAY (32/64 bit)
		mask += flagbit5;        //    NORMALS
		mask += flagbit8;        //    TRIANGLES
		mask += flagbit12;       //    WIREFRAME

		setting += 0;		     //    SINGLE PRECISION (float)
		setting += 0;            //    32 BIT INDEX ARRAY (Int32)
		setting += flagbit5;     //    NORMALS ON
		setting += flagbit8;     //    TRIANGLES ON
		setting += flagbit12;    //    WIREFRAME ON
		setFormat(ifcModel, setting, mask);

		GenerateWireFrameGeometry(ifcModel, ifcObject);
		cleanMemory(ifcModel, 0);

		ifcObject = ifcObject->next;
	}
}

bool	contains(wchar_t * txtI, wchar_t * txtII)
{
	int_t	i = 0;
	while  (txtI[i]  &&  txtII[i]) {
		if	(txtI[i] != txtII[i]) {
			return	false;
		}
		i++;
	}
	if	(txtII[i]) {
		return	false;
	} else {
		return	true;
	}
}

void	initializeTreeItemInterfaceType(STRUCT__SELECTABLE__TREEITEM * treeItem, int_t * pGeometryItemCheckedCnt, int_t * pGeometryItemUncheckedCnt)
{
	ASSERT((* pGeometryItemCheckedCnt) == 0  &&  (* pGeometryItemUncheckedCnt) == 0);

	while  (treeItem) {
		int_t	childGeometryItemCheckedCnt = 0, childGeometryItemUncheckedCnt = 0;
		initializeTreeItemInterfaceType(treeItem->child, &childGeometryItemCheckedCnt, &childGeometryItemUncheckedCnt);
		(* pGeometryItemCheckedCnt) += childGeometryItemCheckedCnt;
		(* pGeometryItemUncheckedCnt) += childGeometryItemUncheckedCnt;

		switch  (treeItem->interfaceType) {
			case  TYPE_IFCOBJECT_NON_VISIBLE:
				treeItem->select = ITEM_NONE;
				break;
			case  TYPE_IFCOBJECT_VISIBLE:
				treeItem->select = ITEM_CHECKED;
				break;
			case  TYPE_IFCOBJECT_PROPERTYSET:
				treeItem->select = ITEM_PROPERTY_SET;
				break;
			case  TYPE_IFCOBJECT_PROPERTY:
				treeItem->select = ITEM_PROPERTY;
				break;
			case  TYPE_IFCOBJECT_UNKNOWN:
				if	(childGeometryItemCheckedCnt  ||  childGeometryItemUncheckedCnt  ||  (treeItem->ifcObject  &&  (treeItem->ifcObject->noPrimitivesForFaces  ||  treeItem->ifcObject->noPrimitivesForWireFrame))) {
					treeItem->interfaceType = TYPE_IFCOBJECT_VISIBLE;
					if	(treeItem->ifcObject) {
						if	(treeItem->ifcObject->ifcItemCheckedAtStartup) {
							treeItem->select = ITEM_CHECKED;
							(*pGeometryItemCheckedCnt)++;
						} else {
							treeItem->select = ITEM_UNCHECKED;
							(*pGeometryItemUncheckedCnt)++;
						}
					} else {
						if	(childGeometryItemCheckedCnt) {
							if	(childGeometryItemUncheckedCnt) {
								treeItem->select = ITEM_PARTLY_CHECKED;
							} else {
								treeItem->select = ITEM_CHECKED;
							}
						} else {
							ASSERT(childGeometryItemUncheckedCnt);
							treeItem->select = ITEM_UNCHECKED;
						}
					}
				} else {
					treeItem->interfaceType = TYPE_IFCOBJECT_NON_VISIBLE;
					treeItem->select = ITEM_NONE;
				}
				break;
			default:
				ASSERT(false);
				break;
		}

		treeItem = treeItem->next;
	}
}

bool	charContains(wchar_t * txtI, wchar_t * txtII)
{
	int_t	i = 0;
	while  (txtI[i]) {
		int_t	j = 0;
		while  (txtII[j]  &&  txtI[i+j] == txtII[j]) {
			j++;
		}
		if	(!txtII[j]) { return  true; }
		i++;
	}
	return	false;
}

wchar_t	* charFill(wchar_t * txtI, wchar_t * txtII, wchar_t * buffer)
{
	int_t	i = 0;
	while  (txtI[i]) {
		int_t	j = 0;
		while  (txtII[j]  &&  txtI[i+j] == txtII[j]) {
			j++;
		}
		if	(!txtII[j]) {
			i += j;
			ASSERT(txtI[i+0] == '=');
			ASSERT(txtI[i+1] == '"');
			i += 2;
			j = 0;
			while  (txtI[i + j] != '"') {
				buffer[j++] = txtI[i + j];
			}
			buffer[j] = 0;
			return	buffer;
		}
		i++;
	}
	return	false;
}


bool	charFind(wchar_t * txtI, wchar_t * txtII)
{
	int_t	i = 0;
	while  (txtI[i]  &&  txtI[i] != '>') {
		int_t	j = 0;
		while  (txtII[j]  &&  txtI[i+j] == txtII[j]) {
			j++;
		}
		if	(!txtII[j]) {
			return	true;
		}
		i++;
	}
	return	false;
}

void	CleanupIfcFile()
{
	if	(globalIfcModel) {
		sdaiCloseModel(globalIfcModel);
		globalIfcModel = 0;
	}

	if	(ifcObjectsLinkedList  ||  baseTreeItem) {
		RemoveIfcObjects();
		ifcObjectsLinkedList = 0;

		RemoveNestedTreeItem(baseTreeItem);
		baseTreeItem = 0;
	}
}

void	finalizeMaterial()
{
	STRUCT_MATERIAL	* material = firstMaterial;

	int	arraySize = 0;
	while  (material) {
		arraySize++;
		material = material->next;
	}

	material = firstMaterial;
	while  (material) {
		ASSERT(material->active);

		ASSERT(material->MTRL == 0);
		material->MTRL = (void *) new D3DMATERIAL9;

		((D3DMATERIAL9 *) material->MTRL)->Ambient.r = material->ambient.R;
		((D3DMATERIAL9 *) material->MTRL)->Ambient.g = material->ambient.G;
		((D3DMATERIAL9 *) material->MTRL)->Ambient.b = material->ambient.B;
		((D3DMATERIAL9 *) material->MTRL)->Ambient.a = (float) material->transparency;
		((D3DMATERIAL9 *) material->MTRL)->Diffuse.r = material->diffuse.R;
		((D3DMATERIAL9 *) material->MTRL)->Diffuse.g = material->diffuse.G;
		((D3DMATERIAL9 *) material->MTRL)->Diffuse.b = material->diffuse.B;
		((D3DMATERIAL9 *) material->MTRL)->Diffuse.a = (float) material->transparency;
		((D3DMATERIAL9 *) material->MTRL)->Specular.r = material->specular.R;
		((D3DMATERIAL9 *) material->MTRL)->Specular.g = material->specular.G;
		((D3DMATERIAL9 *) material->MTRL)->Specular.b = material->specular.B;
		((D3DMATERIAL9 *) material->MTRL)->Specular.a = (float) material->transparency;
		((D3DMATERIAL9 *) material->MTRL)->Emissive.r = material->ambient.R / 2;
		((D3DMATERIAL9 *) material->MTRL)->Emissive.g = material->ambient.G / 2;
		((D3DMATERIAL9 *) material->MTRL)->Emissive.b = material->ambient.B / 2;
		((D3DMATERIAL9 *) material->MTRL)->Emissive.a = (float) material->transparency / 2;
		((D3DMATERIAL9 *) material->MTRL)->Power = 0.5;

		mtrls.push_back((D3DMATERIAL9 *) material->MTRL);
		material = material->next;
	}
}

bool	ParseIfcFile(CWnd* pParent)
{
CProgressWnd wndProgress(pParent);

wndProgress.SetRange(0,100);//10);
wndProgress.SetText(L"Loading IFC file...\n\nParsing file from disk ...");

//	for (int i = 0; i < TEST_RANGE; i++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();

if (wndProgress.Cancelled()) {
	//MessageBox(pParent, L"Progress Cancelled");
	//break;
}
//	}	


	firstItemWithGeometryPassed = false;
	CleanupIfcFile();

	setStringUnicode(1);
	int_t	ifcModel = sdaiOpenModelBNUnicode(0, (char*) ifcFileName, 0);

for  (int m = 0; m < 2; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}
	if	(ifcModel) {
		FILE	* file = 0;

		//
		//	Check if this file is an IFC2x3 file or IFC4
		//

		wchar_t	* fileSchema = 0;
		GetSPFFHeaderItem(ifcModel, 9, 0, sdaiUNICODE, (char**) &fileSchema);
		if	(fileSchema == 0  ||
			 contains(fileSchema, L"IFC2x3")  ||
			 contains(fileSchema, L"IFC2X3")  ||
			 contains(fileSchema, L"IFC2x2")  ||
			 contains(fileSchema, L"IFC2X2")  ||
			 contains(fileSchema, L"IFC2x_")  ||
			 contains(fileSchema, L"IFC2X_")  ||
			 contains(fileSchema, L"IFC20")) {
			sdaiCloseModel(ifcModel);

for  (int m = 0; m < 2; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

			ifcModel = sdaiOpenModelBNUnicode(0, (char*) ifcFileName, (char*) ifcSchemaName_IFC2x3);

for  (int m = 0; m < 10; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

			if	(!ifcModel) { return  false; }

			_wfopen_s(&file, xmlSettings_IFC2x3, L"r");
		} else {
			if	(contains(fileSchema, L"IFC4")  ||
				 contains(fileSchema, L"IFC2x4")  ||
				 contains(fileSchema, L"IFC2X4")) {
				sdaiCloseModel(ifcModel);

for  (int m = 0; m < 2; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

				ifcModel = sdaiOpenModelBNUnicode(0, (char*) ifcFileName, (char*) ifcSchemaName_IFC4);

for  (int m = 0; m < 10; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

				if	(!ifcModel) { return  false; }

				_wfopen_s(&file, xmlSettings_IFC4, L"r");
			} else {
				sdaiCloseModel(ifcModel);
				return	false;
			}
		}

wndProgress.SetText(L"Loading IFC file...\n\nFind objects in Database ...");

		if	(file) {
			bool	hide;

			STRUCT__IFC__OBJECT	** firstFreeIfcObject = &ifcObjectsLinkedList;
			ASSERT((*firstFreeIfcObject) == 0);

			wchar_t	buff[512], buffName[512];
			while  (0 != fgetws(buff, 500, file)) {
				if	(charContains(buff, L"<object ")) {
					charFill(buff, L"name", buffName);
					hide = charFind(buff, L"hide");

					firstFreeIfcObject = queryIfcObjects(ifcModel, firstFreeIfcObject, buffName, hide);
				}
			}
			fclose(file);
		}

for  (int m = 0; m < 3; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

		if	(ifcObjectsLinkedList) {
			initializeMaterial(ifcModel);

wndProgress.SetText(L"Loading IFC file...\n\nCreate geometry ...");

			GenerateGeometryNestedCall(ifcModel, &wndProgress, 80);

wndProgress.SetText(L"Loading IFC file...\n\nAttach material ...");

			finalizeMaterial();

wndProgress.SetText(L"Loading IFC file...\n\nPrepare interface ...");

			CreateTreeItem_ifcProject(ifcModel, &baseTreeItem);

			if	(baseTreeItem) {
				baseTreeItem->next = CreateTreeItem(0, 0, L"Header Info", 0, L"", L"");
				baseTreeItem->next->headerSet = GetHeaderInfo(ifcModel);
				baseTreeItem->next->select = ITEM_PROPERTY_SET;
				baseTreeItem->next->interfaceType = TYPE_IFCOBJECT_PROPERTYSET;
			}
		}

		CreateTreeItem_nonReferencedIfcItems();

		int_t	checkedItems = 0, uncheckedItems = 0;
		initializeTreeItemInterfaceType(baseTreeItem, &checkedItems, &uncheckedItems);

		globalIfcModel = ifcModel;

for  (int m = 0; m < 3; m++) {
wndProgress.StepIt();
wndProgress.PeekAndPump();
}

		return	true;
	} else {
		ASSERT(ifcObjectsLinkedList == 0  &&  baseTreeItem == 0);
		return	false;
	}
}
