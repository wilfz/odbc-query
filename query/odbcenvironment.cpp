
#include "odbcenvironment.h"
#include <cassert>

using namespace linguversa;
using namespace std;


ODBCEnvironment::ODBCEnvironment()
{
    SQLRETURN retcode = SQL_ERROR;
    retcode = ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
    if (! SQL_SUCCEEDED(retcode))
        throw DbException( retcode, SQL_HANDLE_ENV, m_henv);
      
    if (m_henv == SQL_NULL_HENV)
        throw(new exception());
    
    // Set ODBC version
    retcode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION,
                                            (void *) SQL_OV_ODBC3, 0);
    if (! SQL_SUCCEEDED(retcode))
        throw DbException( retcode, SQL_HANDLE_ENV, m_henv);
}

ODBCEnvironment::~ODBCEnvironment()
{
    SQLRETURN retcode = ::SQLFreeHandle( SQL_HANDLE_ENV, m_henv);
    if ( SQL_SUCCEEDED(retcode))
        m_henv = SQL_NULL_HENV;
}

SQLRETURN ODBCEnvironment::FetchDriverName(string& driverdescription, SQLUSMALLINT direction)
{
   if (m_henv == SQL_NULL_HENV)
        return false;
        
    const int buflen1 = SQL_MAX_MESSAGE_LENGTH;
    SQLTCHAR description[buflen1];
    SQLSMALLINT usedlen1 = 0;
    const int buflen2 = SQL_MAX_MESSAGE_LENGTH;
    SQLSMALLINT usedlen2 = 0;

    SQLRETURN retcode = ::SQLDrivers( m_henv, direction, 
        description, buflen1, &usedlen1,
        nullptr, buflen2, &usedlen2);
        
    if (SQL_SUCCEEDED(retcode))
    {
        driverdescription.assign((const char*) description); // TODO: Unicode
    }
    else if(retcode != SQL_NO_DATA)
    {
        throw DbException(retcode, SQL_HANDLE_ENV, m_henv);
    }

    return retcode;
}

SQLRETURN ODBCEnvironment::FetchDriverInfo(string& driverdescription, vector<string>& attributes, SQLUSMALLINT direction)
{
   if (m_henv == SQL_NULL_HENV)
        return false;
        
    driverdescription.clear();
    attributes.clear();
    
    const int buflen1 = SQL_MAX_MESSAGE_LENGTH;
    SQLTCHAR description[buflen1];
    SQLSMALLINT usedlen1 = 0;
    const int buflen2 = SQL_MAX_MESSAGE_LENGTH;
    SQLTCHAR attrib[buflen2];
    SQLSMALLINT usedlen2 = 0;

    SQLRETURN retcode = ::SQLDrivers( m_henv, direction, 
        description, buflen1, &usedlen1,
        attrib, buflen2, &usedlen2);
        
    if (SQL_SUCCEEDED(retcode))
    {
        driverdescription.assign((const char*) description); // TODO: Unicode
        string s;
        SQLSMALLINT i = 0;
        while (i < usedlen2 && attrib[i] != 0)
        {
            s.assign((const char*) attrib+i); // TODO: Unicode
            attributes.resize(attributes.size()+1,s);
            i += (SQLSMALLINT) s.size() + 1;
        }
    }
    else if(retcode != SQL_NO_DATA)
    {
        throw DbException(retcode, SQL_HANDLE_ENV, m_henv);
    }

    return retcode;
}

SQLRETURN ODBCEnvironment::FetchDataSourceInfo(string& dsn, string& drivername, SQLUSMALLINT direction)
{
   if (m_henv == SQL_NULL_HENV)
        return false;
        
    const int buflen1 = SQL_MAX_MESSAGE_LENGTH;
    SQLTCHAR dsname[buflen1];
    SQLSMALLINT usedlen1 = 0;
    const int buflen2 = SQL_MAX_MESSAGE_LENGTH;
    SQLTCHAR driver[buflen2];
    SQLSMALLINT usedlen2 = 0;

    SQLRETURN retcode = ::SQLDataSources( m_henv, direction, 
        dsname, buflen1, &usedlen1,
        driver, buflen2, &usedlen2);
        
    if (SQL_SUCCEEDED(retcode))
    {
        dsn.assign((const char*) dsname); // TODO: Unicode
        drivername.assign((const char*) driver); // TODO: Unicode
    }
    else if(retcode != SQL_NO_DATA)
    {
        throw DbException(retcode, SQL_HANDLE_ENV, m_henv);
    }

    return retcode;
}
