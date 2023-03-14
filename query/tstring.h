#pragma once

#include <string>

#ifdef _WIN32
#include <tchar.h>
#define swprintf swprintf_s

#ifdef UNICODE
#define tstring wstring
#define tcout wcout
#define tcerr wcerr
#else
#define tstring string
#define tcout cout
#define tcerr cerr
#endif

// needs to be included above sql.h for windows
#ifndef __MINGW32__
#define NOMINMAX
#if _MSC_VER <= 1500
#define nullptr NULL
#define noexcept
#define snprintf sprintf_s
#endif // _MSC_VER <= 1500
#endif
#include <windows.h>

#else
// Non-Windows PLatforms
#define tstring string
#define tcout cout
#define tcerr cerr
#define _tprintf printf
#define _T
#endif
