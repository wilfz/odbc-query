#pragma once

#include <string>

#ifdef _WIN32
#include <tchar.h>
#define swprintf swprintf_s

#ifdef UNICODE
#define tstring wstring
#define tcout wcout
#else
#define tstring string
#define tcout cout
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
#define _tprintf printf
#define _T
#endif


namespace linguversa
{

std::tstring lower(const std::tstring& str);
std::tstring upper(const std::tstring& str);

class lvstring : public std::tstring
{
public:
    lvstring() {};
    lvstring(std::tstring s) : std::tstring(s) {};
    lvstring(const TCHAR s[]) : std::tstring(s) {};
    
    void Replace(const std::tstring from, const std::tstring to);
    std::tstring Left(const unsigned int n) const;
    std::tstring Right(const unsigned int n) const;
    std::tstring Mid(const unsigned int from, const unsigned int cnt) const;
    std::tstring& MakeLower();
    std::tstring& MakeUpper();
};

}
