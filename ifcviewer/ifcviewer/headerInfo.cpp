#include "stdafx.h"
#include "headerInfo.h"
#include "generic.h"


STRUCT__HEADER__SET	* CreateHeaderSet(STRUCT__HEADER__SET * parent, wchar_t * name, wchar_t * value)
{
	STRUCT__HEADER__SET	* headerSet = new STRUCT__HEADER__SET;

	headerSet->structType = STRUCT_TYPE_HEADER_SET;
	headerSet->name = __copy__(name);
	headerSet->value = __copy__(value);

	headerSet->hTreeItem = 0;

	headerSet->nameBuffer = new wchar_t[512];

	if	(parent) {
		STRUCT__HEADER__SET	** ppHeaderSet = &parent->child;
		while  ((*ppHeaderSet)) {
			ppHeaderSet = &(*ppHeaderSet)->next;
		}
		(*ppHeaderSet) = headerSet;
	}

	headerSet->child = 0;
	headerSet->next = 0;

	return	headerSet;
}


//
//		Get Header Info
//


STRUCT__HEADER__SET	* GetHeaderDescription(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerDescription = CreateHeaderSet(parent, L"Set of Descriptions", 0),
						** ppHeader = &headerDescription->child;

	wchar_t	* text = 0;
	int_t	i = 0;

	if	(!GetSPFFHeaderItem(ifcModel, 0, i, sdaiUNICODE, (char **) &text)) {
		while  (!GetSPFFHeaderItem(ifcModel, 0, i++, sdaiUNICODE, (char **) &text)) {
			(* ppHeader) = CreateHeaderSet(headerDescription, L"Description", text);
			ppHeader = &(* ppHeader)->next;
			text = 0;
		}
	}

	return	headerDescription;
}

STRUCT__HEADER__SET	* GetImplementationLevel(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerImplementationLevel = 0;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 1, 0, sdaiUNICODE, (char **) &text);
	headerImplementationLevel = CreateHeaderSet(parent, L"ImplementationLevel", text);

	return	headerImplementationLevel;
}

STRUCT__HEADER__SET	* GetName(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerName = 0;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 2, 0, sdaiUNICODE, (char **) &text);
	headerName = CreateHeaderSet(parent, L"Name", text);

	return	headerName;
}

STRUCT__HEADER__SET	* GetTimeStamp(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerTimeStamp = nullptr;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 3, 0, sdaiUNICODE, (char **) &text);
	headerTimeStamp = CreateHeaderSet(parent, L"TimeStamp", text);

	return	headerTimeStamp;
}

STRUCT__HEADER__SET	* GetAuthor(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerAuthor = CreateHeaderSet(parent, L"Set of Authors", 0),
						** ppHeader = &headerAuthor->child;

	wchar_t	* text = nullptr;
	int_t	i = 0;

	if	(!GetSPFFHeaderItem(ifcModel, 4, i, sdaiUNICODE, (char **) &text)) {
		while  (!GetSPFFHeaderItem(ifcModel, 4, i++, sdaiUNICODE, (char **) &text)) {
			(* ppHeader) = CreateHeaderSet(headerAuthor, L"Author", text);
			ppHeader = &(* ppHeader)->next;
			text = 0;
		}
	}

	return	headerAuthor;
}

STRUCT__HEADER__SET	* GetOrganization(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerOrganization = CreateHeaderSet(parent, L"Set of Organizations", 0),
						** ppHeader = &headerOrganization->child;

	wchar_t	* text = nullptr;
	int_t	i = 0;

	if	(!GetSPFFHeaderItem(ifcModel, 5, i, sdaiUNICODE, (char **) &text)) {
		while  (!GetSPFFHeaderItem(ifcModel, 5, i++, sdaiUNICODE, (char **) &text)) {
			(* ppHeader) = CreateHeaderSet(headerOrganization, L"Organization", text);
			ppHeader = &(* ppHeader)->next;
			text = 0;
		}
	}

	return	headerOrganization;
}

STRUCT__HEADER__SET	* GetPreprocessorVersion(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerPreprocessorVersion = nullptr;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 6, 0, sdaiUNICODE, (char **) &text);
	headerPreprocessorVersion = CreateHeaderSet(parent, L"PreprocessorVersion", text);

	return	headerPreprocessorVersion;
}

STRUCT__HEADER__SET	* GetOriginatingSystem(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerOriginatingSystem = nullptr;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 7, 0, sdaiUNICODE, (char **) &text);
	headerOriginatingSystem = CreateHeaderSet(parent, L"OriginatingSystem", text);

	return	headerOriginatingSystem;
}

STRUCT__HEADER__SET	* GetAuthorization(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerAuthorization = nullptr;

	wchar_t	* text = nullptr;

	GetSPFFHeaderItem(ifcModel, 8, 0, sdaiUNICODE, (char **) &text);
	headerAuthorization = CreateHeaderSet(parent, L"Authorization", text);

	return	headerAuthorization;
}

STRUCT__HEADER__SET	* GetFileSchema(int_t ifcModel, STRUCT__HEADER__SET * parent)
{
	STRUCT__HEADER__SET	* headerFileSchema = CreateHeaderSet(parent, L"Set of FileSchemas", 0),
						** ppHeader = &headerFileSchema->child;

	wchar_t	* text = nullptr;
	int_t	i = 0;

	if	(!GetSPFFHeaderItem(ifcModel, 9, i, sdaiUNICODE, (char **) &text)) {
		while  (!GetSPFFHeaderItem(ifcModel, 9, i++, sdaiUNICODE, (char **) &text)) {
			(* ppHeader) = CreateHeaderSet(headerFileSchema, L"FileSchema", text);
			ppHeader = &(* ppHeader)->next;
			text = 0;
		}
	}

	return	headerFileSchema;
}

STRUCT__HEADER__SET	* GetHeaderInfo(int_t ifcModel)
{
	STRUCT__HEADER__SET	* headerFileSchema = CreateHeaderSet(0, L"Header Info", 0),
						** ppHeader = &headerFileSchema->child;

	(* ppHeader) = GetHeaderDescription(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetImplementationLevel(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetName(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetTimeStamp(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetAuthor(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetOrganization(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetPreprocessorVersion(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetOriginatingSystem(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetAuthorization(ifcModel, headerFileSchema);
	ppHeader = &(* ppHeader)->next;
	(* ppHeader) = GetFileSchema(ifcModel, headerFileSchema);

	return	headerFileSchema;
}
 