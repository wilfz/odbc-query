#pragma once

#include "tstring.h"

#include <sql.h>
#include <sqlext.h>

using namespace std;

namespace linguversa
{
class DbException : public exception 
{
public:
    DbException( SQLRETURN ret, SQLSMALLINT handleType, SQLHANDLE handle);
    ~DbException();

    virtual const char* what() const noexcept;

    SQLRETURN getSqlCode() const;
    tstring getSqlState() const;
    SQLINTEGER getNativeError() const;
    tstring getSqlErrorMessage() const;

protected:
    SQLRETURN m_SqlReturn;
    SQLTCHAR m_SqlState[8];
    SQLINTEGER m_NativeError;
    SQLTCHAR m_SqlError[SQL_MAX_MESSAGE_LENGTH];
    char m_what[SQL_MAX_MESSAGE_LENGTH * 2];
};

}

