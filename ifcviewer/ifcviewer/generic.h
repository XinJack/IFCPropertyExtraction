#pragma once

#include "ifcengine/include/engdef.h"
#include "unit.h"
//#include <stdint.h>


#define		STRUCT_TYPE_MATERIAL				200
#define		STRUCT_TYPE_OBJECT_COLOR			201
#define		STRUCT_TYPE_HEADER_SET				202
#define		STRUCT_TYPE_IFC_ITEM				205
#define		STRUCT_TYPE_SELECTABLE_TREEITEM		206



struct STRUCT__SELECTABLE__TREEITEM;
struct STRUCT__COLOR;							//		color
struct STRUCT__HEADER__SET;						//		headerInfo
struct STRUCT__SIUNIT;


struct STRUCT_MATERIALS;


typedef struct VECTOR3 {
	double							x;
	double							y;
	double							z;
}	VECTOR3;

typedef struct STRUCT__BASE {
	int_t							structType;
}	STRUCT_BASE;

struct STRUCT__IFC__OBJECT {
	int_t							structType;
	int_t							ifcInstance;
	int_t							ifcEntity;
	bool							ifcItemCheckedAtStartup;

	wchar_t							* globalId;
	wchar_t							* ifcType;
	wchar_t							* name;
	wchar_t							* description;

	STRUCT__SELECTABLE__TREEITEM	* treeItemGeometry;
	STRUCT__SELECTABLE__TREEITEM	* treeItemProperties;
	STRUCT__PROPERTY__SET			* propertySets;
	bool							propertySetsAvailableButNotLoadedYet;

	STRUCT_MATERIALS				* materials;
	STRUCT__SIUNIT					* units;

	STRUCT__IFC__OBJECT				* next;

	VECTOR3							vecMin;
	VECTOR3							vecMax;

	int_t							noVertices;
	float							* vertices;

	int_t							noPrimitivesForFaces;
	int32_t							* indicesForFaces;
	int_t							vertexOffsetForFaces;
	int_t							indexOffsetForFaces;

	int_t							noPrimitivesForWireFrame;
	int32_t							* indicesForLinesWireFrame;
	int_t							vertexOffsetForWireFrame;
	int_t							indexOffsetForWireFrame;
};

struct STRUCT__SELECTABLE__TREEITEM {
	int_t							structType;

	wchar_t							* globalId;
	wchar_t							* ifcType;
	wchar_t							* name;
	wchar_t							* description;

	int_t							ifcInstance;

	bool							ignore;

	STRUCT__SELECTABLE__TREEITEM	* parent;
	STRUCT__SELECTABLE__TREEITEM	* child;
	STRUCT__SELECTABLE__TREEITEM	* next;

	int_t							interfaceType;

	HTREEITEM						hTreeItem;

	wchar_t							* nameBuffer;
	int32_t							select;
	
	STRUCT__HEADER__SET				* headerSet;
	STRUCT__IFC__OBJECT				* ifcObject;

	bool							showIfcObjectWireFrame;
	bool							showIfcObjectSolid;
	bool							showPlane;

	int_t							vertexOffset;
	int_t							indexOffset;
};



bool	equals(wchar_t * txtI, wchar_t * txtII);
bool	equals(char * txtI, char * txtII);

wchar_t	* __copy__(wchar_t * txt);
