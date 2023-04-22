#pragma once

#include <string>

#ifdef _WIN32
// various Window platforms:
// - Multi_Byte character set or UNICODE
// - x86 or x64 architecture
// - MSVC or MINGW compiler
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <tchar.h>
#define swprintf swprintf_s
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno

#ifdef UNICODE
#define tstring wstring
#define tcout wcout
#define tcerr wcerr
#define tcin wcin
#define tstringstream wstringstream
#define tostream wostream
#define tofstream wofstream
#define tistream wistream
#define tifstream wifstream
#else
#define tstring string
#define tcout cout
#define tcerr cerr
#define tcin cin
#define tstringstream stringstream
#define tostream ostream
#define tofstream ofstream
#define tistream istream
#define tifstream ifstream
#endif

#ifndef __MINGW32__
#define NOMINMAX
#if _MSC_VER <= 1500
#define nullptr NULL
#define noexcept
#define snprintf sprintf_s
#endif // _MSC_VER <= 1500
#endif
// needs to be included above sql.h for windows
#include <windows.h>

#else
// Non-Windows PLatforms
#define tstring string
#define tcout cout
#define tcerr cerr
#define tcin cin
#define tstringstream stringstream
#define tostream ostream
#define tofstream ofstream
#define tistream istream
#define tifstream ifstream
#define _tprintf printf
#define _tstoi atoi
#define _stat64 stat
#define _tstat64 stat
#define _tgetenv getenv
#define _T

#define SI_NO_CONVERSION
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif
