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

/*
using namespace std;

#ifdef UNICODE
class tstring : public wstring
{
public:
    tstring() {};
    tstring(wstring s) : wstring(s) {};
    tstring(const wchar_t s[]) : wstring(s) {};
    
    void Replace(const wstring from, const wstring to);
    wstring Left(const unsigned int n);
    wstring Right(const unsigned int n);
    wstring Mid(const unsigned int from, const unsigned int cnt);
};
#else
class tstring : public string 
{
public:
    tstring() {};
    tstring(string s) : string(s) {};
    tstring(const char s[]) : string(s) {};
    
    void Replace(const string from, const string to);
    string Left(const unsigned int n);
    string Right(const unsigned int n);
    string Mid(const unsigned int from, const unsigned int cnt);
};
#endif
*/
