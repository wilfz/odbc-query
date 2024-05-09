
#include "connection.h"
#include <cassert>

using namespace linguversa;
using namespace std;

SQLHENV Connection::m_henv = SQL_NULL_HENV;
int Connection::m_ConnectionCounter = 0;

Connection::Connection()
{
    m_hdbc = NULL;
    if (m_henv == SQL_NULL_HENV)
    {
        SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
        if (! SQL_SUCCEEDED(retcode))
            throw DbException( retcode, SQL_HANDLE_ENV, m_henv);
        
        retcode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (void *) SQL_OV_ODBC3, 0);
        if (! SQL_SUCCEEDED(retcode)) 
            throw DbException( retcode, SQL_HANDLE_ENV, m_henv);

        // SQL_ATTR_METADATA_ID: The default value is SQL_FALSE.
        // The arguments of catalog functions can either contain a string search pattern or not, depending on the argument. 
        // Case is significant.
        // SQL_ATTR_METADATA_ID can also be set on the statement level. 
        // (It is the only connection attribute that is also a statement attribute.)
    }
    
    if (m_henv != SQL_NULL_HENV)
    {
        m_ConnectionCounter++;
    }
    else
    {
        throw(new exception());
    }
}

Connection::~Connection()
{
    if (m_hdbc != SQL_NULL_HDBC)
        Close();
    
    if (m_ConnectionCounter > 0)
        m_ConnectionCounter--;
    else
        m_ConnectionCounter = 0;
        
    if (m_ConnectionCounter == 0)
    {
        SQLRETURN retcode = ::SQLFreeHandle( SQL_HANDLE_ENV, m_henv);
        if ( SQL_SUCCEEDED(retcode))
            m_henv = SQL_NULL_HENV;
    }
}

bool Connection::Open(tstring connectionstring)
{
    if (m_henv == SQL_NULL_HENV || m_hdbc != SQL_NULL_HDBC)
        return false;
        
    SQLRETURN retcode = ::SQLAllocConnect(m_henv, &m_hdbc);
    if (! SQL_SUCCEEDED(retcode) || m_hdbc == SQL_NULL_HDBC)
        throw DbException( retcode, SQL_HANDLE_ENV, m_henv);

    // length of the input buffer in bytes or Unicode characters
    const int buflen = SQL_MAX_MESSAGE_LENGTH;
    SQLSMALLINT usedlen = 0;
    SQLTCHAR* pszConnectOutput = new SQLTCHAR[buflen];
    retcode = ::SQLDriverConnect( m_hdbc, NULL, (SQLTCHAR*) connectionstring.c_str(), SQL_NTS, 
        pszConnectOutput, buflen, 
        &usedlen, SQL_DRIVER_COMPLETE);

    delete[] pszConnectOutput;

    if (!SQL_SUCCEEDED(retcode))
    {
        // ::SQLDisconnect(m_hdbc); <-- would cause another error because we are not connected
        // but we still need m_hdbc for a qualified error message.
        throw DbException(retcode, SQL_HANDLE_DBC, m_hdbc);
    }

    return SQL_SUCCEEDED(retcode) && usedlen <= buflen && m_hdbc != SQL_NULL_HDBC;
}

bool Connection::IsOpen() const
{
    return m_henv != SQL_NULL_HENV && m_hdbc != SQL_NULL_HDBC;
}

void Connection::Close()
{
    if (m_hdbc != SQL_NULL_HDBC)
    {
        SQLRETURN retcode = ::SQLDisconnect(m_hdbc);
        // !his yields -1 if we are not yet connected!
        //assert( SQL_SUCCEEDED(retcode));
        retcode = ::SQLFreeConnect(m_hdbc);
        m_hdbc = SQL_NULL_HDBC;
        assert( SQL_SUCCEEDED(retcode));
        // prevent unused variable warning/error in release mode
        (void)retcode;
    }
}

SQLRETURN Connection::SqlGetDriverName(tstring& drivername)
{
    if (m_hdbc == NULL)
        return SQL_INVALID_HANDLE;

    SQLRETURN nRetCode = SQL_SUCCESS;
    SQLSMALLINT nBufNeeded = 32;
    SQLSMALLINT nBufLen = 0;
    do {
        nBufLen = nBufNeeded + 1;
        SQLTCHAR* pBuf = new SQLTCHAR[nBufLen];
        // use SQLGetInfo() function to find out about the driver
        nRetCode = ::SQLGetInfo(m_hdbc, SQL_DRIVER_NAME, pBuf, nBufLen, &nBufNeeded);
        drivername.assign( (const TCHAR*) pBuf);
        delete[] pBuf;
    } while (SQL_SUCCEEDED(nRetCode) && nBufLen <= nBufNeeded);

    return nRetCode;
}

SQLRETURN Connection::SqlGetDriverVersion(tstring& driverversion)
{
    if (m_hdbc == NULL)
        return SQL_INVALID_HANDLE;

    SQLRETURN nRetCode = SQL_SUCCESS;
    SQLSMALLINT nBufNeeded = 32;
    SQLSMALLINT nBufLen = 0;
    do {
        nBufLen = nBufNeeded + 1;
        SQLTCHAR* pBuf = new SQLTCHAR[nBufLen];
        // use SQLGetInfo() function to find out about the driver
        nRetCode = ::SQLGetInfo(m_hdbc, SQL_DRIVER_VER, pBuf, nBufLen, &nBufNeeded);
        driverversion.assign( (const TCHAR*) pBuf);
        delete[] pBuf;
    } while (SQL_SUCCEEDED(nRetCode) && nBufLen <= nBufNeeded);

    return nRetCode;
}
