/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "data.h"
#include "languageprovider.hh"
#include "utf8.h"

#include <string>

HINSTANCE LanguageResources;


bool Language_Provider_Load_String(int id, char * destination, std::size_t capacity)
{
	if (destination == NULL || capacity == 0) {
		return(false);
	}

	if (LanguageResources == NULL) {
		Init_Language_Resources(false);
	}

	if (LoadString(LanguageResources, id, destination, (int)capacity) == 0) {
		return(false);
	}
	destination[capacity - 1] = '\0';

	// Windows before 10 version 1903 ignores the manifest and narrows to its own code page.
	if (GetACP() != CP_UTF8) {
		std::string text = UTF8::From_Windows_1252(destination);
		UTF8::Copy(destination, capacity, text.c_str());
	}
	return(true);
}


void const * Fetch_Resource(LPCSTR resname, LPCSTR restype)
{
	HRSRC handle = FindResource(LanguageResources, resname, restype);
	if (handle == NULL) {
		return(NULL);
	}

	HGLOBAL rhandle = LoadResource(LanguageResources, handle);
	if (rhandle == NULL) {
		return(NULL);
	}

	return(LockResource(rhandle));
}


bool Init_Language_Resources(bool show_error)
{
	if (LanguageResources == NULL) {
		LanguageResources = LoadLibrary("Language.dll");
		if (LanguageResources == NULL) {
			if (show_error == true) {
				MessageBox(NULL,
					"Unable to initialize Language.dll, please reinstall Tiberian Sun.\n"
					"Keine Initialisierung von Language.DLL m\xF6glich. Bitte installieren Sie Tiberian Sun erneut.\n"
					"Initialisation de Language.DLL impossible. Veuillez r\xE9installer Tiberian Sun.",
					"Tiberian Sun",
					MB_ICONERROR);
			}
			return(false);
		}
	}

	return(true);
}


void Get_Language_Version(char *version_string)
{
	INT dwSize;
	LPVOID pFileInfo;
	UINT puInfoLen;
	LPCTSTR pcData;
	DWORD dwHandle;

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *pvInfo;

	char szQuery[128];
	char szFile[MAX_PATH];

	if (version_string != NULL) {
		version_string[0] = '\0';

		if (LanguageResources != NULL && GetModuleFileName(LanguageResources, szFile, sizeof(szFile)) > 0) {
			dwHandle = 1;
			dwSize = GetFileVersionInfoSize(szFile, &dwHandle);

			if (dwSize > 0) {
				pFileInfo = new char[dwSize];

				if (pFileInfo != NULL) {
					if (GetFileVersionInfo(szFile, dwHandle, dwSize, pFileInfo)) {
						VerQueryValue(pFileInfo, TEXT("\\VarFileInfo\\Translation"), (LPVOID *)&pvInfo, &puInfoLen);

						if (puInfoLen > 0) {
							sprintf(szQuery, TEXT("\\StringFileInfo\\%04X%04X\\InternalName"), pvInfo->wLanguage, pvInfo->wCodePage);
							VerQueryValue(pFileInfo, szQuery, (LPVOID *)&pcData, &puInfoLen);

							if (puInfoLen > 0) {
								sprintf(version_string, "Language: %s ", pcData);
								sprintf(szQuery, TEXT("\\StringFileInfo\\%04X%04X\\FileVersion"), pvInfo->wLanguage, pvInfo->wCodePage);
								VerQueryValue(pFileInfo, szQuery, (LPVOID *)&pcData, &puInfoLen);

								if (puInfoLen > 0) {
									strcat(version_string, pcData);
								}
							}
						}
					}

					delete [] pFileInfo;
				}
			}
		}
	}
}
