#pragma once

#include "tstring.h"

// helper to manipulate strings in MFC style
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
