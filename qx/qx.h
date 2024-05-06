#pragma once

#include "../query/tstring.h"

#ifdef _WIN32
// various Window platforms:
// - Multi_Byte character set or UNICODE
// - x86 or x64 architecture
// - MSVC or MINGW compiler
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno

#ifdef UNICODE
#define tcout wcout
#define tcerr wcerr
#define tcin wcin
#define tstringstream wstringstream
#define tostream wostream
#define tofstream wofstream
#define tistream wistream
#define tifstream wifstream
#else
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
#endif

#else
// Non-Windows PLatforms
#define tcout cout
#define tcerr cerr
#define tcin cin
#define tstringstream stringstream
#define tostream ostream
#define tofstream ofstream
#define tistream istream
#define tifstream ifstream
#define _tprintf printf
#define _stat64 stat
#define _tstat64 stat
#define _tgetenv getenv
#define SI_NO_CONVERSION
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif
