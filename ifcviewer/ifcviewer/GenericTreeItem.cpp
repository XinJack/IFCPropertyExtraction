#include "stdafx.h"
#include "GenericTreeItem.h"
#include "generic.h"

#include <math.h>


extern	STRUCT__IFC__OBJECT	* ifcObjectsLinkedList;



STRUCT__IFC__OBJECT	* CreateIfcObject(int_t ifcEntity, int_t ifcInstance, wchar_t * ifcType, bool hide, wchar_t * globalId, wchar_t * name, wchar_t * description)
{
	STRUCT__IFC__OBJECT	* ifcObject = new STRUCT__IFC__OBJECT;

	ifcObject->structType = STRUCT_TYPE_IFC_ITEM;
	ifcObject->ifcType = __copy__(ifcType);
	ifcObject->globalId = __copy__(globalId);
	ifcObject->name = __copy__(name);
	ifcObject->description = __copy__(description);

	ifcObject->propertySets = 0;
	ifcObject->propertySetsAvailableButNotLoadedYet = false;

	ifcObject->noVertices = 0;
	ifcObject->vertices = 0;

	ifcObject->next = 0;
	
	ifcObject->materials = 0;

	ifcObject->vecMin.x = 0;
	ifcObject->vecMin.y = 0;
	ifcObject->vecMin.z = 0;
	ifcObject->vecMax.x = 0;
	ifcObject->vecMax.y = 0;
	ifcObject->vecMax.z = 0;

	ifcObject->ifcInstance = ifcInstance;
	ifcObject->ifcEntity = ifcEntity;
	if	(hide) {
		ifcObject->ifcItemCheckedAtStartup = false;
	} else {
		ifcObject->ifcItemCheckedAtStartup = true;
	}
	ifcObject->treeItemGeometry = 0;
	ifcObject->treeItemProperties = 0;

	ifcObject->noVertices = 0;
	ifcObject->vertices = 0;

	ifcObject->noPrimitivesForFaces = 0;
	ifcObject->indicesForFaces = 0;
	ifcObject->vertexOffsetForFaces = 0;
	ifcObject->indexOffsetForFaces = 0;

	ifcObject->noPrimitivesForWireFrame = 0;
	ifcObject->indicesForLinesWireFrame = 0;
	ifcObject->vertexOffsetForWireFrame = 0;
	ifcObject->indexOffsetForWireFrame = 0;

	return	ifcObject;
}

STRUCT__SELECTABLE__TREEITEM	* CreateTreeItem(STRUCT__SELECTABLE__TREEITEM * parent)
{
	STRUCT__SELECTABLE__TREEITEM	* treeItem = new STRUCT__SELECTABLE__TREEITEM();

	treeItem->structType = STRUCT_TYPE_SELECTABLE_TREEITEM;
	treeItem->headerSet = 0;
	treeItem->ifcObject = 0;

	treeItem->ignore = false;

	treeItem->globalId = 0;
	treeItem->name = 0;
	treeItem->description = 0;
	treeItem->ifcType = 0;

	treeItem->nameBuffer = new wchar_t[512];

	treeItem->hTreeItem = 0;
	treeItem->select = ITEM_UNKNOWN;

	treeItem->showIfcObjectSolid = false;
	treeItem->showIfcObjectWireFrame = false;
	treeItem->showPlane = false;

	treeItem->interfaceType = TYPE_IFCOBJECT_UNKNOWN;

	treeItem->indexOffset = 0;
	treeItem->vertexOffset = 0;

	treeItem->parent = parent;
	treeItem->child = 0;
	treeItem->next = 0;

	return	treeItem;
}

STRUCT__SELECTABLE__TREEITEM	* CreateTreeItem(STRUCT__SELECTABLE__TREEITEM * parent, int_t ifcInstance, wchar_t * ifcType, wchar_t * globalId, wchar_t * name, wchar_t * description)
{
	STRUCT__SELECTABLE__TREEITEM	* newTreeItem = CreateTreeItem(parent);

	newTreeItem->ifcType = __copy__(ifcType);
	newTreeItem->globalId = __copy__(globalId);
	newTreeItem->name = __copy__(name);
	newTreeItem->description = __copy__(description);

	newTreeItem->ifcInstance = ifcInstance;

	return	newTreeItem;
}

//
//	Cleanup
//

void	RemoveColor(STRUCT__COLOR * color)
{
	color = color;
	//delete(color);
}

void	RemoveHeaderSet(STRUCT__HEADER__SET * headerSet)
{
	if	(headerSet->name) {
		delete(headerSet->name);
	}

	if	(headerSet->value) {
		delete(headerSet->value);
	}

	delete(headerSet->nameBuffer);

	delete(headerSet);
}

void	RemoveNestedHeaderSet(STRUCT__HEADER__SET * headerSet)
{
	while  (headerSet) {
		RemoveNestedHeaderSet(headerSet->child);

		STRUCT__HEADER__SET	* headerSetToRemove = headerSet;
		headerSet = headerSet->next;

		RemoveHeaderSet(headerSetToRemove);
	}
}

void	RemoveIfcProperty(STRUCT__PROPERTY * ifcProperty)
{
	if	(ifcProperty->name) {
		delete(ifcProperty->name);
	}
	if	(ifcProperty->description) {
		delete(ifcProperty->description);
	}

	if	(ifcProperty->nominalValue) {
		delete(ifcProperty->nominalValue);
	}
	if	(ifcProperty->areaValue) {
		delete(ifcProperty->areaValue);
	}
	if	(ifcProperty->unit) {
		delete(ifcProperty->unit);
	}

	delete(ifcProperty->nameBuffer);

	delete(ifcProperty);
}

void	RemoveNestedIfcProperty(STRUCT__PROPERTY * ifcProperty)
{
	while  (ifcProperty) {
		STRUCT__PROPERTY	* ifcPropertyToRemove = ifcProperty;
		ifcProperty = ifcProperty->next;

		RemoveIfcProperty(ifcPropertyToRemove);
	}
}

void	RemoveIfcPropertySet(STRUCT__PROPERTY__SET * ifcPropertySet)
{
	if	(ifcPropertySet->name) {
		delete(ifcPropertySet->name);
	}
	if	(ifcPropertySet->description) {
		delete(ifcPropertySet->description);
	}
	delete(ifcPropertySet->nameBuffer);

	delete(ifcPropertySet);
}

void	RemoveNestedIfcPropertySet(STRUCT__PROPERTY__SET * ifcPropertySet)
{
	while  (ifcPropertySet) {
		STRUCT__PROPERTY__SET	* ifcPropertySetToRemove = ifcPropertySet;
		ifcPropertySet = ifcPropertySet->next;

		RemoveIfcPropertySet(ifcPropertySetToRemove);
	}
}

void	RemoveIfcObject(STRUCT__IFC__OBJECT * ifcObject)
{
	if	(ifcObject->ifcType) {
		delete	ifcObject->ifcType;
	}
	if	(ifcObject->globalId) {
		delete	ifcObject->globalId;
	}
	if	(ifcObject->name) {
		delete	ifcObject->name;
	}
	if	(ifcObject->description) {
		delete	ifcObject->description;
	}

	if	(ifcObject->vertices) {
		delete	ifcObject->vertices;
	}
	if	(ifcObject->indicesForFaces) {
		delete	ifcObject->indicesForFaces;
	}
	if	(ifcObject->indicesForLinesWireFrame) {
		delete	ifcObject->indicesForLinesWireFrame;
	}

	delete	ifcObject;
}

void	RemoveIfcObjects()
{
	STRUCT__IFC__OBJECT	* ifcObject = ifcObjectsLinkedList;
	while  (ifcObject) {
		STRUCT__IFC__OBJECT	* ifcObjectToRemove = ifcObject;
		ifcObject = ifcObject->next;
		RemoveIfcObject(ifcObjectToRemove);
	}
	ifcObjectsLinkedList = nullptr;
}

void	RemoveTreeItem(STRUCT__SELECTABLE__TREEITEM * treeItem)
{
	if	(treeItem->ifcType) {
		delete	treeItem->ifcType;
	}
	if	(treeItem->globalId) {
		delete	treeItem->globalId;
	}
	if	(treeItem->name) {
		delete	treeItem->name;
	}
	if	(treeItem->description) {
		delete	treeItem->description;
	}
	
	delete	treeItem->nameBuffer;

	delete	treeItem;
}

void	RemoveNestedTreeItem(STRUCT__SELECTABLE__TREEITEM * treeItem)
{
	while  (treeItem) {
		RemoveNestedTreeItem(treeItem->child);

		STRUCT__SELECTABLE__TREEITEM	* treeItemToRemove = treeItem;
		treeItem = treeItem->next;

		RemoveTreeItem(treeItemToRemove);
	}
}
 