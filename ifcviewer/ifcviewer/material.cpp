#include "stdafx.h"
#include "material.h"

#include <list>

#include "ifcengine\include\IFCEngine.h"
#include "ifcengine\include\engine.h"

#ifdef WIN64
	#define int_t int64_t
#else
	#define int_t int32_t
#endif



//
//	Structures used only internally within Material.cpp/.h
//



struct STRUCT_MATERIAL_META_INFO {
	bool							isPoint;
	bool							isEdge;
	bool							isShell;

	int64_t							expressID;
	STRUCT_MATERIAL					* material;

	STRUCT_MATERIAL_META_INFO		* child;
	STRUCT_MATERIAL_META_INFO		* next;
};



void	getRGB_shapeRepresentation(int_t model, int_t ifcShapeRepresentationInstance, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo);
void	minimizeMaterialItems(STRUCT_MATERIAL_META_INFO * materialMetaInfo, STRUCT_MATERIAL ** ppMaterial, bool * pUnique, bool * pDefaultColorIsUsed);
bool	DEBUG_colorLoopConsistanceCheck();

void	walkThroughGeometry__transformation(int64_t owlModel, int64_t owlInstance, STRUCT_MATERIALS *** ppMaterials, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo);


STRUCT_MATERIAL				* firstMaterial, * lastMaterial, * defaultMaterial;
STRUCT_MATERIAL_META_INFO	* DEBUG__localObjectColor;


int_t	totalAllocatedMaterial = 0;

void	DEBUG__activeMaterial(STRUCT_MATERIAL_META_INFO * materialMetaInfo)
{
	while  (materialMetaInfo) {
		DEBUG__activeMaterial(materialMetaInfo->child);

//		ASSERT(materialMetaInfo->material == 0  ||  materialMetaInfo->material->active == false  ||  materialMetaInfo->material->deleted == false);
		ASSERT(materialMetaInfo->material == 0  ||  materialMetaInfo->material->active == false);

		materialMetaInfo = materialMetaInfo->next;
	}
}

STRUCT_MATERIALS	* new_STRUCT_MATERIALS(STRUCT_MATERIAL * material)
{
	STRUCT_MATERIALS	* materials = new STRUCT_MATERIALS;

	materials->indexArrayOffset = 0;
	materials->indexArrayPrimitives = 0;
	materials->material = material;
	materials->next = nullptr;

	return	materials;
}

STRUCT_MATERIALS	* new_STRUCT_MATERIALS(STRUCT_MATERIAL * material, int_t ifcModel, int_t ifcInstance)
{
	STRUCT_MATERIALS	* materials = new_STRUCT_MATERIALS(material);

	int_t	setting = 0, mask = 0;
	mask += 4096;//flagbit12;       //    WIREFRAME
	setting += 0;            //    WIREFRAME OFF
	setFormat(ifcModel, setting, mask);

	int64_t	vertexBufferSize = 0, indexBufferSize = 0;
	CalculateInstance((int64_t) ifcInstance, &vertexBufferSize, &indexBufferSize, 0);

	materials->indexArrayOffset = 0;
	materials->indexArrayPrimitives = indexBufferSize / 3;

	return	materials;
}

void	deleteMaterial(STRUCT_MATERIAL * material)
{
	totalAllocatedMaterial--;

	ASSERT(material->active == false);

	ASSERT(DEBUG_colorLoopConsistanceCheck());

	STRUCT_MATERIAL	* prev = material->prev,
					* next = material->next;

	if	(prev == 0) {
		ASSERT(firstMaterial == next->prev);
	}

	if	(next == 0) {
		ASSERT(lastMaterial == prev->next);
	}

	if	(prev) {
		ASSERT(prev->next == material);
		prev->next = next;
	} else {
		ASSERT(firstMaterial == material);
		firstMaterial = next;
		next->prev = 0;
	}

	if	(next) {
		ASSERT(next->prev == material);
		next->prev = prev;
	} else {
		ASSERT(lastMaterial == material);
		lastMaterial = prev;
		ASSERT(prev->next == 0);
	}

	material->active = false;

//#ifdef	DEBUG
//	material->deleted = true;
//#else
	delete	material;
//#endif
}

STRUCT_MATERIAL		* newMaterial()
{
	totalAllocatedMaterial++;

	STRUCT_MATERIAL	* material = new STRUCT_MATERIAL;

//	material->structType = STRUCT_TYPE_MATERIAL;

	material->ambient.R = -1;
	material->ambient.G = -1;
	material->ambient.B = -1;
	material->ambient.A = -1;

	material->diffuse.R = -1;
	material->diffuse.G = -1;
	material->diffuse.B = -1;
	material->diffuse.A = -1;

	material->specular.R = -1;
	material->specular.G = -1;
	material->specular.B = -1;
	material->specular.A = -1;

	material->emissive.R = -1;
	material->emissive.G = -1;
	material->emissive.B = -1;
	material->emissive.A = -1;

	material->transparency = -1;
	material->shininess = -1;

	material->next = 0;
	material->prev = lastMaterial;

	if	(firstMaterial == 0) {
		ASSERT(lastMaterial == 0);
		firstMaterial = material;
	}
	lastMaterial = material;
	if	(lastMaterial->prev) {
		lastMaterial->prev->next = lastMaterial;
	}

	material->MTRL = nullptr;
	material->active = false;
//#ifdef	DEBUG
//	material->deleted = false;
//#endif

	return	material;
}

int64_t	rdfClassTransformation,
		rdfClassCollection,
		owlDataTypePropertyExpressID,
		owlObjectTypePropertyMatrix,
		owlObjectTypePropertyObject,
		owlObjectTypePropertyObjects;

void	initializeMaterial(int_t ifcModel)
{
	firstMaterial = 0;
	lastMaterial = 0;

	defaultMaterial = newMaterial();

	defaultMaterial->ambient.R = 0;
	defaultMaterial->ambient.G = 1.f;
	defaultMaterial->ambient.B = 0;
	defaultMaterial->ambient.A = 0;
	defaultMaterial->diffuse.R = 0;
	defaultMaterial->diffuse.G = 0.8f;
	defaultMaterial->diffuse.B = 0;
	defaultMaterial->diffuse.A = 0;
	defaultMaterial->specular.R = 0;
	defaultMaterial->specular.G = 0.8f;
	defaultMaterial->specular.B = 0;
	defaultMaterial->specular.A = 0;
	defaultMaterial->emissive.R = 0;
	defaultMaterial->emissive.G = 0.8f;
	defaultMaterial->emissive.B = 0;
	defaultMaterial->emissive.A = 0;

	defaultMaterial->transparency = 1;
	defaultMaterial->shininess = 1;

	defaultMaterial->active = true;

	rdfClassTransformation = GetClassByName((int64_t) ifcModel, "Transformation");
	rdfClassCollection = GetClassByName((int64_t) ifcModel, "Collection");

	owlDataTypePropertyExpressID = GetPropertyByName((int64_t) ifcModel, "expressID");

	owlObjectTypePropertyMatrix = GetPropertyByName((int64_t) ifcModel, "matrix");
	owlObjectTypePropertyObject = GetPropertyByName((int64_t) ifcModel, "object");
	owlObjectTypePropertyObjects = GetPropertyByName((int64_t) ifcModel, "objects");
}

STRUCT_MATERIAL_META_INFO	* newMaterialMetaInfo(int_t ifcInstance)
{
	STRUCT_MATERIAL_META_INFO	* materialMetaInfo = new STRUCT_MATERIAL_META_INFO;

	if	(ifcInstance) {
		materialMetaInfo->expressID = internalGetP21Line(ifcInstance);
	} else {
		materialMetaInfo->expressID = -1;
	}

	materialMetaInfo->isPoint = false;
	materialMetaInfo->isEdge = false;
	materialMetaInfo->isShell = false;
	
	materialMetaInfo->material = newMaterial();

	materialMetaInfo->child = 0;
	materialMetaInfo->next = 0;

	return	materialMetaInfo;
}


//
//
//	colour RGB
//
//


void	getRGB_colourRGB(int_t SurfaceColourInstance, STRUCT_COLOR * color)
{
	double	red = 0, green = 0, blue = 0;
	sdaiGetAttrBN(SurfaceColourInstance, (char*) L"Red", sdaiREAL, &red);
	sdaiGetAttrBN(SurfaceColourInstance, (char*) L"Green", sdaiREAL, &green);
	sdaiGetAttrBN(SurfaceColourInstance, (char*) L"Blue", sdaiREAL, &blue);

	color->R = (float) red;
	color->G = (float) green;
	color->B = (float) blue;
}

void	getRGB_surfaceStyle(
#ifdef _DEBUG
				int_t			model,
#endif
				int_t			surfaceStyleInstance,
				STRUCT_MATERIAL	* material
			)
{
	int_t	* styles_set = nullptr, styles_cnt;
	sdaiGetAttrBN(surfaceStyleInstance, (char*) L"Styles", sdaiAGGR, &styles_set);
	styles_cnt = sdaiGetMemberCount(styles_set);
	for  (int_t i = 0; i < styles_cnt; i++) {
		int_t	surfaceStyleRenderingInstance = 0, SurfaceColourInstance = 0, SpecularColourInstance = 0, DiffuseColourInstance = 0;
		engiGetAggrElement(styles_set, i, sdaiINSTANCE, &surfaceStyleRenderingInstance);
#ifdef _DEBUG
		int_t	surfaceStyleRenderingEntity = sdaiGetEntity(model, (char*) L"IFCSURFACESTYLERENDERING");
#endif
		ASSERT(sdaiGetInstanceType(surfaceStyleRenderingInstance) == surfaceStyleRenderingEntity);

		double	transparency = 0;
		sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"Transparency", sdaiREAL, &transparency);
		material->transparency = 1 - (float) transparency;

		sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"SurfaceColour", sdaiINSTANCE, &SurfaceColourInstance);
		if (SurfaceColourInstance != 0)
		{
			getRGB_colourRGB(SurfaceColourInstance, &material->ambient);
		} else {
			ASSERT(false);
		}

		sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"DiffuseColour", sdaiINSTANCE, &DiffuseColourInstance);
		if (DiffuseColourInstance != 0) {
			getRGB_colourRGB(DiffuseColourInstance, &material->diffuse);
		} else {
			int_t	ADB = 0;
			sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"DiffuseColour", sdaiADB, &ADB);
			if (ADB) {
				double	value = 0;
				sdaiGetADBValue((void *) ADB, sdaiREAL, &value);
				material->diffuse.R = (float) value * material->ambient.R;
				material->diffuse.G = (float) value * material->ambient.G;
				material->diffuse.B = (float) value * material->ambient.B;
			}
		}

		sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"SpecularColour", sdaiINSTANCE, &SpecularColourInstance);
		if	(SpecularColourInstance != 0) {
			getRGB_colourRGB(SpecularColourInstance, &material->specular);
		} else {
			int_t	ADB = 0;
			sdaiGetAttrBN(surfaceStyleRenderingInstance, (char*) L"SpecularColour", sdaiADB, &ADB);
			if (ADB) {
				double	value = 0;
				sdaiGetADBValue((void *) ADB, sdaiREAL, &value);
				material->specular.R = (float) value * material->ambient.R;
				material->specular.G = (float) value * material->ambient.G;
				material->specular.B = (float) value * material->ambient.B;
			}
		}
	}
}

void	getRGB_presentationStyleAssignment(
#ifdef _DEBUG
				int_t			model,
#endif
				int_t			presentationStyleAssignmentInstance,
				STRUCT_MATERIAL	* material
			)
{
	int_t	* styles_set = nullptr, styles_cnt, i = 0;
	sdaiGetAttrBN(presentationStyleAssignmentInstance, (char*) L"Styles", sdaiAGGR, &styles_set);
	styles_cnt = sdaiGetMemberCount(styles_set);
	while  (i < styles_cnt) {
		int_t	surfaceStyleInstance = 0;
		engiGetAggrElement(styles_set, i, sdaiINSTANCE, &surfaceStyleInstance);
		if  (surfaceStyleInstance != 0) {
			getRGB_surfaceStyle(
#ifdef _DEBUG
					model,
#endif
					surfaceStyleInstance,
					material
				);
		}
		i++;
	}
}

void	getRGB_styledItem(
#ifdef _DEBUG
				int_t			model,
#endif
				int_t			ifcStyledItemInstance,
				STRUCT_MATERIAL	* material
			)
{
	int_t	* styles_set = nullptr, styles_cnt, i = 0;
	sdaiGetAttrBN(ifcStyledItemInstance, (char*) L"Styles", sdaiAGGR, &styles_set);
	styles_cnt = sdaiGetMemberCount(styles_set);
	while  (i < styles_cnt) {
		int_t presentationStyleAssignmentInstance = 0;
		engiGetAggrElement(styles_set, i, sdaiINSTANCE, &presentationStyleAssignmentInstance);
		if	(presentationStyleAssignmentInstance != 0) {
			getRGB_presentationStyleAssignment(
#ifdef _DEBUG
					model,
#endif
					presentationStyleAssignmentInstance,
					material
				);
		}
		i++;
	}
}

void	searchDeeper(int_t model, int_t geometryInstance, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo, STRUCT_MATERIAL * material)
{
	int_t	styledItemInstance = 0;
	sdaiGetAttrBN(geometryInstance, (char*) L"StyledByItem", sdaiINSTANCE, &styledItemInstance);
	if	(styledItemInstance != 0) {
		getRGB_styledItem(
#ifdef _DEBUG
				model,
#endif
				styledItemInstance,
				material
			);
		if (material->ambient.R >= 0) {
			return;
		}
	}

	int_t	booleanClippingResultEntity = sdaiGetEntity(model, (char*) L"IFCBOOLEANCLIPPINGRESULT"),
			booleanResultEntity = sdaiGetEntity(model, (char*) L"IFCBOOLEANRESULT"),
			mappedItemEntity = sdaiGetEntity(model, (char*) L"IFCMAPPEDITEM"),
			shellBasedSurfaceModelEntity = sdaiGetEntity(model, (char*) L"IFCSHELLBASEDSURFACEMODEL");
	if	(sdaiGetInstanceType(geometryInstance) == booleanResultEntity  ||  sdaiGetInstanceType(geometryInstance) == booleanClippingResultEntity) {
		int_t	geometryChildInstance = 0;
		sdaiGetAttrBN(geometryInstance, (char*) L"FirstOperand", sdaiINSTANCE, &geometryChildInstance);
		if	(geometryChildInstance) {
			searchDeeper(model, geometryChildInstance, ppMaterialMetaInfo, material);
		}
	} else if  (sdaiGetInstanceType(geometryInstance) == mappedItemEntity) {
		int_t	representationMapInstance = 0;
		sdaiGetAttrBN(geometryInstance, (char*) L"MappingSource", sdaiINSTANCE, &representationMapInstance);
		int_t	shapeRepresentationInstance = 0;
		sdaiGetAttrBN(representationMapInstance, (char*) L"MappedRepresentation", sdaiINSTANCE, &shapeRepresentationInstance);

		if	(shapeRepresentationInstance) {
			ASSERT((*ppMaterialMetaInfo)->child == 0);
			getRGB_shapeRepresentation(model, shapeRepresentationInstance, &(*ppMaterialMetaInfo)->child);
		}
	} else if  (sdaiGetInstanceType(geometryInstance) == shellBasedSurfaceModelEntity) {
		int_t	* geometryChildAggr = nullptr;
		sdaiGetAttrBN(geometryInstance, (char*) L"SbsmBoundary", sdaiAGGR, &geometryChildAggr);
		STRUCT_MATERIAL_META_INFO	** ppSubMaterialMetaInfo = &(*ppMaterialMetaInfo)->child;

		int_t	cnt = sdaiGetMemberCount(geometryChildAggr), i = 0;
		while  (i < cnt) {
			int_t	geometryChildInstance = 0;
			engiGetAggrElement(geometryChildAggr, i, sdaiINSTANCE, &geometryChildInstance);
			if	(geometryChildInstance) {
				ASSERT((*ppSubMaterialMetaInfo) == 0);
				(*ppSubMaterialMetaInfo) = newMaterialMetaInfo(geometryChildInstance);

				searchDeeper(model, geometryChildInstance, ppSubMaterialMetaInfo, (*ppSubMaterialMetaInfo)->material);
				ppSubMaterialMetaInfo = &(*ppSubMaterialMetaInfo)->next;
			}
			i++;
		}
	}
}

bool	__equals(wchar_t * txtI, wchar_t * txtII)
{
	int_t i = 0;
	if (txtI && txtII) {
		while (txtI[i]) {
			if (txtI[i] != txtII[i]) {
				return	false;
			}
			i++;
		}
		if (txtII[i]) {
			return	false;
		}
	} else if (txtI || txtII) {
		return	false;
	}
	return	true;
}

void	getRGB_shapeRepresentation(int_t model, int_t ifcShapeRepresentationInstance, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo)
{
	wchar_t * pRepresentationIdentifier = nullptr, * RepresentationType = nullptr;
	sdaiGetAttrBN(ifcShapeRepresentationInstance, (char*) L"RepresentationIdentifier", sdaiUNICODE, &pRepresentationIdentifier);
	sdaiGetAttrBN(ifcShapeRepresentationInstance, (char*) L"RepresentationType", sdaiUNICODE, &RepresentationType);

	if ( (__equals(pRepresentationIdentifier, L"Body")  ||  __equals(pRepresentationIdentifier, L"Mesh")  ||  __equals(pRepresentationIdentifier, L"Facetation") )  &&
		 !__equals(RepresentationType, L"BoundingBox") )
	{
		int_t	* geometry_set = nullptr, geometry_cnt, i = 0;
		sdaiGetAttrBN(ifcShapeRepresentationInstance, (char*) L"Items", sdaiAGGR, &geometry_set);
		geometry_cnt = sdaiGetMemberCount(geometry_set);
		i = 0;
		while  (i < geometry_cnt) {
			int_t	geometryInstance = 0, styledItemInstance = 0;
			engiGetAggrElement(geometry_set, i, sdaiINSTANCE, &geometryInstance);

			ASSERT((*ppMaterialMetaInfo) == 0);
			(*ppMaterialMetaInfo) = newMaterialMetaInfo(geometryInstance);
			
			sdaiGetAttrBN(geometryInstance, (char*) L"StyledByItem", sdaiINSTANCE, &styledItemInstance);
			if (styledItemInstance != 0)
			{
				getRGB_styledItem(
#ifdef _DEBUG
						model,
#endif
						styledItemInstance,
						(*ppMaterialMetaInfo)->material
					);
			} else {
				searchDeeper(model, geometryInstance, ppMaterialMetaInfo, (*ppMaterialMetaInfo)->material);
			}

			ppMaterialMetaInfo = &(*ppMaterialMetaInfo)->next;
			i++;
		}
	}
}

void getRGB_productDefinitionShape(int_t model, int_t ifcObjectInstance, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo)
{
	int_t	* representations_set = nullptr, representations_cnt, i = 0;
	sdaiGetAttrBN(ifcObjectInstance, (char*) L"Representations", sdaiAGGR, &representations_set);
	representations_cnt = sdaiGetMemberCount(representations_set);
	while  (i < representations_cnt) {
		int_t	ifcShapeRepresentation = 0;
		engiGetAggrElement(representations_set, i, sdaiINSTANCE, &ifcShapeRepresentation);
		if	(ifcShapeRepresentation != 0) {
			getRGB_shapeRepresentation(model, ifcShapeRepresentation, ppMaterialMetaInfo);
		}
		i++;
	}
}

STRUCT_MATERIALS	* ifcObjectMaterial(int_t ifcModel, int_t ifcInstance)//STRUCT__IFC__OBJECT * ifcObject)
{
	STRUCT_MATERIAL_META_INFO	* materialMetaInfo = nullptr, ** ppMaterialMetaInfo = &materialMetaInfo;
	STRUCT_MATERIAL				* returnedMaterial = nullptr;

	int_t	ifcProductRepresentationInstance = 0;
	sdaiGetAttrBN(ifcInstance, (char*) L"Representation", sdaiINSTANCE, &ifcProductRepresentationInstance);
	if	(ifcProductRepresentationInstance != 0) {
		getRGB_productDefinitionShape(ifcModel, ifcProductRepresentationInstance, ppMaterialMetaInfo);
	}
	
	if	(materialMetaInfo) {
		bool	unique = true, defaultColorIsUsed = false;
		STRUCT_MATERIAL	* material = nullptr;
		DEBUG__localObjectColor = materialMetaInfo;
		minimizeMaterialItems(materialMetaInfo, &material, &unique, &defaultColorIsUsed);

		if	(unique) {
			returnedMaterial = material;
		} else {
			returnedMaterial = 0;
		}

		if	(defaultColorIsUsed) {
			//
			//	Color not found, check if we can find colors via propertySets
			//
/*			STRUCT__PROPERTY__SET	* propertySet = ifcItem->propertySets;
			while  (propertySet) {
				if	(equals(propertySet->name, L"Pset_Draughting")) {
					STRUCT__PROPERTY	* _property = propertySet->properties;
					while  (_property) {
						if	(equals(_property->name, L"Red")) {
							int_t	value = _wtoi(_property->nominalValue);
						}
						if	(equals(_property->name, L"Green")) {
							int_t	value = _wtoi(_property->nominalValue);
					}
						if	(equals(_property->name, L"Blue")) {
							int_t	value = _wtoi(_property->nominalValue);
						}
						_property = _property->next;
					}
				}
				propertySet = propertySet->next;
			}	//	*/
		}
	}

	if	(returnedMaterial) {
		return	new_STRUCT_MATERIALS(returnedMaterial, ifcModel, ifcInstance);
	} else {
		STRUCT_MATERIALS	* materials = 0, ** materialsRef = &materials;
		if	(materialMetaInfo) {
			int_t	setting = 0, mask = 0;
			mask += 4096;//flagbit12;       //    WIREFRAME
			setting += 0;            //    WIREFRAME OFF
//			setting += flagbit12;    //    WIREFRAME ON
			setFormat(ifcModel, setting, mask);

			walkThroughGeometry__transformation((int64_t) ifcModel, (int64_t) ifcInstance, &materialsRef, &materialMetaInfo);
		}

		if	(materials) {
			return	materials;
		} else {
			return	new_STRUCT_MATERIALS(firstMaterial, ifcModel, ifcInstance);
		}
	}
}

bool	DEBUG_colorLoopConsistanceCheck()
{
	STRUCT_MATERIAL	* itemColorLoop;

	//
	//	Check consitancy
	//
	itemColorLoop = firstMaterial;
	while  (itemColorLoop) {
		if	(itemColorLoop->prev) {
			ASSERT(itemColorLoop->prev->next == itemColorLoop);
		} else {
			ASSERT(itemColorLoop == firstMaterial);
		}

		if	(itemColorLoop->next) {
			ASSERT(itemColorLoop->next->prev == itemColorLoop);
		} else {
			ASSERT(itemColorLoop == lastMaterial);
		}

		itemColorLoop = itemColorLoop->next;
	}

	itemColorLoop = lastMaterial;
	while  (itemColorLoop) {
		if	(itemColorLoop->prev) {
			ASSERT(itemColorLoop->prev->next == itemColorLoop);
		} else {
			ASSERT(itemColorLoop == firstMaterial);
		}

		if	(itemColorLoop->next) {
			ASSERT(itemColorLoop->next->prev == itemColorLoop);
		} else {
			ASSERT(itemColorLoop == lastMaterial);
		}

		itemColorLoop = itemColorLoop->prev;
	}

	return	true;
}

void	minimizeMaterialItems(STRUCT_MATERIAL_META_INFO * materialMetaInfo, STRUCT_MATERIAL ** ppMaterial, bool * pUnique, bool * pDefaultColorIsUsed)
{
	while  (materialMetaInfo) {
		//
		//	Check nested child object (i.e. Mapped Items)
		//
		if	(materialMetaInfo->child) {
			ASSERT(materialMetaInfo->material->ambient.R == -1);
			ASSERT(materialMetaInfo->material->active == false);
			deleteMaterial(materialMetaInfo->material);
			materialMetaInfo->material = 0;
			minimizeMaterialItems(materialMetaInfo->child, ppMaterial, pUnique, pDefaultColorIsUsed);
		}

		//
		//	Complete Color
		//
		STRUCT_MATERIAL	* material = materialMetaInfo->material;
		if	(material) {
			if	(material->ambient.R == -1) {
				materialMetaInfo->material = defaultMaterial;
				deleteMaterial(material);
			} else {
				if	(material->diffuse.R == -1) {
					material->diffuse.R = material->ambient.R;
					material->diffuse.G = material->ambient.G;
					material->diffuse.B = material->ambient.B;
					material->diffuse.A = material->ambient.A;
				}
				if	(material->specular.R == -1) {
					material->specular.R = material->ambient.R;
					material->specular.G = material->ambient.G;
					material->specular.B = material->ambient.B;
					material->specular.A = material->ambient.A;
				}
			}
		} else {
			ASSERT(materialMetaInfo->child);
		}

		ASSERT(DEBUG_colorLoopConsistanceCheck());

		//
		//	Merge the same colors
		//
		material = materialMetaInfo->material;
		if	(material  &&  material != defaultMaterial) {
			bool	adjusted = false;
			ASSERT(material->active == false);
			STRUCT_MATERIAL	* materialLoop = firstMaterial->next;
			ASSERT(firstMaterial == defaultMaterial);
			while  (materialLoop) {
				if	( (materialLoop->active)  &&
					  (material->transparency == material->transparency)  &&
					  (material->ambient.R == materialLoop->ambient.R)  &&
					  (material->ambient.G == materialLoop->ambient.G)  &&
					  (material->ambient.B == materialLoop->ambient.B)  &&
					  (material->ambient.A == materialLoop->ambient.A)  &&
					  (material->diffuse.R == materialLoop->diffuse.R)  &&
					  (material->diffuse.G == materialLoop->diffuse.G)  &&
					  (material->diffuse.B == materialLoop->diffuse.B)  &&
					  (material->diffuse.A == materialLoop->diffuse.A)  &&
					  (material->specular.R == materialLoop->specular.R)  &&
					  (material->specular.G == materialLoop->specular.G)  &&
					  (material->specular.B == materialLoop->specular.B)  &&
					  (material->specular.A == materialLoop->specular.A) ) {
					materialMetaInfo->material = materialLoop; 
					deleteMaterial(material);
					materialLoop = 0;
					adjusted = true;
				} else {
					if	(materialLoop->active == false) {
						while  (materialLoop) {
							ASSERT(materialLoop->active == false);
							materialLoop = materialLoop->next;
						}
						materialLoop = 0;
					} else {
						materialLoop = materialLoop->next;
					}
				}
			}

			if	(adjusted) {
				ASSERT(materialMetaInfo->material->active);
			} else {
				ASSERT(materialMetaInfo->material->active == false);
				materialMetaInfo->material->active = true;
			}

			ASSERT(materialLoop == 0  ||  (materialLoop == defaultMaterial  &&  materialLoop->next == 0));
		}
		
		//
		//	Assign default color in case of no color and no children
		//
		if	(materialMetaInfo->material == 0  &&  materialMetaInfo->child == 0) {
			materialMetaInfo->material = defaultMaterial;
		}

		//
		//	Check if unique
		//
		material = materialMetaInfo->material;
		if	((*ppMaterial)) {
			if	((*ppMaterial) != material) {
				if	(material == 0  &&  materialMetaInfo->child) {
				} else {
					(*pUnique) = false;
				}
			}
		} else {
			ASSERT((*pUnique) == true);
			(*ppMaterial) = material;
		}
		
		if	(material == defaultMaterial) {
			(*pDefaultColorIsUsed) = true;
		}

		materialMetaInfo = materialMetaInfo->next;
	}
}






















bool	walkThroughGeometry__object(int64_t model, int64_t owlInstance, int64_t expressId, int64_t depth, int64_t transformationOwlInstance)
{
	int64_t	* owlInstanceExpressID = 0, expressIDCard = 0;
	GetDataTypeProperty(owlInstance, owlDataTypePropertyExpressID, (void **) &owlInstanceExpressID, &expressIDCard);
	if	(expressIDCard == 1) {
		int64_t	expressID = owlInstanceExpressID[0];
		if	(expressId == expressID) {
			SetObjectTypeProperty(transformationOwlInstance, owlObjectTypePropertyObject, &owlInstance, 1);
			return	true;
		} else {
			if	(GetInstanceClass(owlInstance) == rdfClassTransformation) {
				int64_t	* owlInstanceObject = 0, objectCard = 0;
				GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);
				if	(objectCard == 1) {
					int64_t	* owlInstanceMatrix = 0, matrixCard = 0;
					GetObjectTypeProperty(owlInstance, owlObjectTypePropertyMatrix, &owlInstanceMatrix, &matrixCard);

					int64_t	subTransformationOwlInstance = CreateInstance(rdfClassTransformation, 0);
					SetObjectTypeProperty(transformationOwlInstance, owlObjectTypePropertyObject, &subTransformationOwlInstance, 1);
					if	(matrixCard == 1) {
						SetObjectTypeProperty(subTransformationOwlInstance, owlObjectTypePropertyMatrix, owlInstanceMatrix, 1);
					} else {
						ASSERT(false);
					}

					return	walkThroughGeometry__object(model, owlInstanceObject[0], expressId, depth+1, subTransformationOwlInstance);
				} else {
					ASSERT(false);
				}
			} else if  (GetInstanceClass(owlInstance) == rdfClassCollection) {
				int64_t	* owlInstanceObjects = 0, objectsCard = 0;
				GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
				int_t	i = 0;
				while  (i < objectsCard) {
					if	(walkThroughGeometry__object(model, owlInstanceObjects[i], expressId, depth+1, transformationOwlInstance)) {
						return	true;
					}
					i++;
				}
			} else {
				ASSERT(false);
			}
		}
	} else {
		ASSERT(depth);
		if	(GetInstanceClass(owlInstance) == rdfClassTransformation) {
			int64_t	* owlInstanceObject = 0, objectCard = 0;
			GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);

			int64_t	* owlInstanceMatrix = 0, matrixCard = 0;
			GetObjectTypeProperty(owlInstance, owlObjectTypePropertyMatrix, &owlInstanceMatrix, &matrixCard);

			int64_t	subTransformationOwlInstance = CreateInstance(rdfClassTransformation, 0);
			SetObjectTypeProperty(transformationOwlInstance, owlObjectTypePropertyObject, &subTransformationOwlInstance, 1);
			if	(matrixCard == 1) {
				SetObjectTypeProperty(subTransformationOwlInstance, owlObjectTypePropertyMatrix, owlInstanceMatrix, 1);
			} else {
				ASSERT(false);
			}

			if	(objectCard == 1) {
				return	walkThroughGeometry__object(model, owlInstanceObject[0], expressId, depth+1, subTransformationOwlInstance);
			} else {
				ASSERT(false);
			}
		} else if  (GetInstanceClass(owlInstance) == rdfClassCollection) {
			int64_t	* owlInstanceObjects = 0, objectsCard = 0;
			GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
			int_t	i = 0;
			while  (i < objectsCard) {
				if	(walkThroughGeometry__object(model, owlInstanceObjects[i], expressId, depth+1, transformationOwlInstance)) {
					return	true;
				}
				i++;
			}
		} else {
			ASSERT(false);
		}
	}

	return	false;
}

bool	walkThroughGeometry__collection(int64_t model, int64_t owlInstance, int64_t expressId, int64_t transformationOwlInstance)
{
	if	(GetInstanceClass(owlInstance) == rdfClassCollection) {
		int64_t	* owlInstanceObjects = 0, objectsCard = 0;
		GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
		int_t	i = 0;
		while  (i < objectsCard) {
			if	(walkThroughGeometry__collection(model, owlInstanceObjects[i], expressId, transformationOwlInstance)) {
				return	true;
			}
			i++;
		}
	} else {
		return	walkThroughGeometry__object(model, owlInstance, expressId, 0, transformationOwlInstance);
	}

	return	false;
}

int64_t	walkThroughGeometry__transformation(int64_t model, int64_t owlInstance, int64_t expressId)
{
	ASSERT(GetInstanceClass(owlInstance) == rdfClassTransformation);

	int64_t	owlInstanceBase = CreateInstance(rdfClassTransformation, 0);

	int64_t	* owlInstanceMatrix = 0, matrixCard = 0;
	GetObjectTypeProperty(owlInstance, owlObjectTypePropertyMatrix, &owlInstanceMatrix, &matrixCard);
	if	(matrixCard == 1) {
		SetObjectTypeProperty(owlInstanceBase, owlObjectTypePropertyMatrix, owlInstanceMatrix, 1);
	} else {
		ASSERT(false);
	}

	int64_t	* owlInstanceObject = 0, objectCard = 0;
	GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);
	if	(objectCard == 1) {
		if	(walkThroughGeometry__collection(model, owlInstanceObject[0], expressId, owlInstanceBase) == false) {
			ASSERT(false);
		}
	} else {
		ASSERT(false);
	}

	return	owlInstanceBase;
}









void	walkThroughGeometry__object(int64_t owlModel, int64_t owlInstance, STRUCT_MATERIALS *** ppMaterials, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo)//, int_t depth)
{
	int64_t	* owlInstanceExpressID = 0, expressIDCard = 0;
	GetDataTypeProperty(owlInstance, owlDataTypePropertyExpressID, (void **) &owlInstanceExpressID, &expressIDCard);
	if	(expressIDCard == 1) {
		int64_t	expressID = owlInstanceExpressID[0];
		while  ((*ppMaterialMetaInfo)  &&  (*ppMaterialMetaInfo)->expressID != expressID) {
			ppMaterialMetaInfo = &(*ppMaterialMetaInfo)->next;
		}
		if	((*ppMaterialMetaInfo)) {
			ASSERT((*ppMaterialMetaInfo)->expressID == expressID);
			if	((*ppMaterialMetaInfo)->child) {
				STRUCT_MATERIAL_META_INFO	** ppMaterialMetaInfoChild = &(*ppMaterialMetaInfo)->child;
				if	(GetInstanceClass(owlInstance) == rdfClassTransformation) {
					int64_t	* owlInstanceObject = 0, objectCard = 0;
					GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);
					if	(objectCard == 1) {
						walkThroughGeometry__object(owlModel, owlInstanceObject[0], ppMaterials, ppMaterialMetaInfoChild);
					} else {
						ASSERT(false);
					}
				} else if  (GetInstanceClass(owlInstance) == rdfClassCollection) {
					int64_t	* owlInstanceObjects = 0, objectsCard = 0;
					GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
					for  (int_t i = 0; i < objectsCard; i++) {
						walkThroughGeometry__object(owlModel, owlInstanceObjects[i], ppMaterials, ppMaterialMetaInfoChild);
					}
				} else {
					ASSERT(false);
				}
			} else {
				//
				//	Now recreate the geometry for this object
				//
				int64_t	vertexBufferSize = 0, indexBufferSize = 0, offset = 0;
				CalculateInstance(owlInstance, &vertexBufferSize, &indexBufferSize, 0);
				if	((**ppMaterials)) {
					offset = (**ppMaterials)->indexArrayOffset + (**ppMaterials)->indexArrayPrimitives * 3;
					(*ppMaterials) = &(**ppMaterials)->next;
				}
				(**ppMaterials) = new_STRUCT_MATERIALS((*ppMaterialMetaInfo)->material);
				(**ppMaterials)->indexArrayOffset = offset;
				(**ppMaterials)->indexArrayPrimitives = indexBufferSize / 3;
			}
			ppMaterialMetaInfo = &(*ppMaterialMetaInfo)->next;
		} else {
			ASSERT(false);
		}
	} else {
		if	(GetInstanceClass(owlInstance) == rdfClassTransformation) {
			int64_t	* owlInstanceObject = 0, objectCard = 0;
			GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);

			if	(objectCard == 1) {
				walkThroughGeometry__object(owlModel, owlInstanceObject[0], ppMaterials, ppMaterialMetaInfo);
			} else {
				ASSERT(false);
			}
		} else if  (GetInstanceClass(owlInstance) == rdfClassCollection) {
			int64_t	* owlInstanceObjects = 0, objectsCard = 0;
			GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
			int_t	i = 0;
			while  (i < objectsCard) {
				walkThroughGeometry__object(owlModel, owlInstanceObjects[i], ppMaterials, ppMaterialMetaInfo);
				i++;
			}
		} else {
			ASSERT(false);
		}
	}
}

void	walkThroughGeometry__collection(int64_t owlModel, int64_t owlInstance, STRUCT_MATERIALS *** ppMaterials, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo)
{
	if	(GetInstanceClass(owlInstance) == rdfClassCollection) {
		int64_t	* owlInstanceObjects = 0, objectsCard = 0;
		GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObjects, &owlInstanceObjects, &objectsCard);
		for  (int_t i = 0; i < objectsCard; i++) {
			walkThroughGeometry__object(owlModel, owlInstanceObjects[i], ppMaterials, ppMaterialMetaInfo);
		}
	} else {
		walkThroughGeometry__object(owlModel, owlInstance, ppMaterials, ppMaterialMetaInfo);
	}
}

void	walkThroughGeometry__transformation(int64_t owlModel, int64_t owlInstance, STRUCT_MATERIALS *** ppMaterials, STRUCT_MATERIAL_META_INFO ** ppMaterialMetaInfo)
{
	ASSERT(GetInstanceClass(owlInstance) == rdfClassTransformation);

	int64_t	* owlInstanceObject = 0, objectCard = 0;
	GetObjectTypeProperty(owlInstance, owlObjectTypePropertyObject, &owlInstanceObject, &objectCard);
	if	(objectCard == 1) {
		walkThroughGeometry__collection(owlModel, owlInstanceObject[0], ppMaterials, ppMaterialMetaInfo);
	} else {
		ASSERT(false);
	}
}

#undef int_t
