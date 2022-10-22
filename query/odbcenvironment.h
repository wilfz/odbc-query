#pragma once

#include "tstring.h"

#include <sql.h>
#include "odbcexception.h"
#include <vector>

using namespace std;

namespace linguversa
{
    class ODBCEnvironment
    {
    public:
        ODBCEnvironment();
        ~ODBCEnvironment();
        
        SQLRETURN FetchDriverName(tstring& driverdescription, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDriverInfo(tstring& driverdescription, vector<tstring>& attributes, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        SQLRETURN FetchDataSourceInfo(tstring& dsn, tstring& drivername, SQLUSMALLINT direction = SQL_FETCH_NEXT);
        
    protected:
        HENV m_henv;
    };
}
