#pragma once

#ifdef _WIN32
// needs to be included above sql.h for windows
#ifndef __MINGW32__
#define NOMINMAX
#if _MSC_VER <= 1500
#define nullptr NULL
#endif // _MSC_VER <= 1500
#endif
#include <windows.h>
#endif

#include <sql.h>
#include "odbcexception.h"
#include <string>
#include <vector>

using namespace std;

namespace linguversa
{
    class ODBCEnvironment
    {
    public:
        ODBCEnvironment();
        ~ODBCEnvironment();
        
        SQLRETURN FetchDriverName(string& driverdescription, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDriverInfo(string& driverdescription, vector<string>& attributes, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDataSourceInfo(string& dsn, string& drivername, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        
    protected:
        HENV m_henv;
    };
}