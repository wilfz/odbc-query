
#include "odbcexception.h"
#include <stdio.h>

using namespace linguversa;

DbException::DbException( SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle)
{
    m_SqlReturn = ret;
    m_SqlState[0] = 0;
    m_NativeError = 0;
    m_SqlError[0] = 0;
    m_what[0] = 0;

    SQLSMALLINT n = 1;
    SQLSMALLINT msgLen;
    ::SQLGetDiagRec( handleType, handle, n, m_SqlState, &m_NativeError, m_SqlError, SQL_MAX_MESSAGE_LENGTH, &msgLen);

#ifdef _UNICODE
    // The virtual member function what() of base class exception returns char* 
    // Thus we have to convert: 
    wchar_t buf[SQL_MAX_MESSAGE_LENGTH * 2];  // m_SqlError und der Rest ...
    swprintf(buf, SQL_MAX_MESSAGE_LENGTH * 2, _T("%0d\t%0d\t%s\t%s\n"), getSqlCode(), (int) getNativeError(), getSqlState().c_str(), getSqlErrorMessage().c_str());
    for (int i = 0; i < SQL_MAX_MESSAGE_LENGTH * 2; i++)
    {
        wchar_t c = buf[i]; 
        m_what[i] = (c <= 255) ? c : '?';
        if (c == 0)
            break;
    }
#else
    snprintf(m_what, SQL_MAX_MESSAGE_LENGTH * 2, "%0d\t%0d\t%s\t%s\n", getSqlCode(), (int) getNativeError(), getSqlState().c_str(), getSqlErrorMessage().c_str());
#endif
}

DbException::~DbException()
{
}

const char* DbException::what() const noexcept
{
    return m_what;
}

SQLRETURN DbException::getSqlCode() const
{
    return m_SqlReturn;
}

tstring DbException::getSqlState() const
{
    tstring sqlstate;
    sqlstate.assign((const TCHAR*) m_SqlState);
    return sqlstate;
}

SQLINTEGER DbException::getNativeError() const
{
    return m_NativeError;
}

tstring DbException::getSqlErrorMessage() const
{
    tstring s;
    s.assign((const TCHAR*) m_SqlError);
    return s;
}

