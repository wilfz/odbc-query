#pragma once

#ifdef _WIN32
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
#endif

#include <sql.h>
#include <sqlext.h>
#include <string>

using namespace std;

namespace linguversa
{
class DbException : exception 
{
public:
    DbException( SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle);
    ~DbException();

    virtual const char* what() const noexcept;

    SQLRETURN getSqlCode() const;
    string getSqlState() const;
    SQLINTEGER getNativeError() const;
    string getSqlErrorMessage() const;

protected:
    SQLRETURN m_SqlReturn;
    SQLTCHAR m_SqlState[8];
    SQLINTEGER m_NativeError;
    SQLTCHAR m_SqlError[SQL_MAX_MESSAGE_LENGTH];
    char m_what[SQL_MAX_MESSAGE_LENGTH * 2];
};

}

