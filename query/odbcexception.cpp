
#include "odbcexception.h"
#include <stdio.h>

using namespace linguversa;

DbException::DbException( SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle)
{
    m_SqlReturn = ret;
    m_SqlState[0] = 0;
    m_NativeError = 0;
    m_SqlError = new SQLTCHAR[SQL_MAX_MESSAGE_LENGTH];
    m_SqlError[0] = 0;
    m_what = new char[SQL_MAX_MESSAGE_LENGTH * 2];  // m_SqlError und der Rest ...
    m_what[0] = 0;
    
    SQLSMALLINT n = 1;
    SQLSMALLINT msgLen;
    ::SQLGetDiagRec( handleType, handle, n, m_SqlState, &m_NativeError, m_SqlError, SQL_MAX_MESSAGE_LENGTH, &msgLen);
    snprintf(m_what, SQL_MAX_MESSAGE_LENGTH * 2, "%0d\t%0d\t%s\t%s\n", getSqlCode(), (int) getNativeError(), getSqlState().c_str(), getSqlErrorMessage().c_str());
}

DbException::~DbException()
{
    delete[] m_SqlError;
    delete[] m_what;
}

const char* DbException::what() const noexcept
{
    return m_what;
}

SQLRETURN DbException::getSqlCode() const
{
    return m_SqlReturn;
}

string DbException::getSqlState() const
{
    string sqlstate;
    sqlstate.assign((const char*) m_SqlState); // TODO Unicode
    return sqlstate;
}

SQLINTEGER DbException::getNativeError() const
{
    return m_NativeError;
}

string DbException::getSqlErrorMessage() const
{
    string s;
    s.assign((const char*) m_SqlError); // TODO Unicode
    return s;
}
