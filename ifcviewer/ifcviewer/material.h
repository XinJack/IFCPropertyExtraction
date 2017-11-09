#pragma once


#include "ifcengine/include/engdef.h"
//#include <stdint.h>


struct STRUCT_COLOR {
	float							R;
	float							G;
	float							B;
	float							A;
};

struct STRUCT_MATERIAL {
	bool							active;

	STRUCT_COLOR					ambient;
	STRUCT_COLOR					diffuse;
	STRUCT_COLOR					specular;
	STRUCT_COLOR					emissive;

	double							transparency;
	double							shininess;

	void							* MTRL;

	STRUCT_MATERIAL					* next;
	STRUCT_MATERIAL					* prev;
};

struct STRUCT_MATERIALS {
	int64_t							indexArrayOffset;
	int64_t							indexArrayPrimitives;

	int64_t							indexOffsetForFaces;

	STRUCT_MATERIAL					* material;

	STRUCT_MATERIALS				* next;
};


void				initializeMaterial(int_t owlModel);
STRUCT_MATERIALS	* ifcObjectMaterial(int_t ifcModel, int_t ifcInstance);
//void				finalizeMaterial();
