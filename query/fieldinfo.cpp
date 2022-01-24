#include "fieldinfo.h"

using namespace linguversa;

FieldInfo::FieldInfo()
{
    m_nCType = DEFAULT_FIELD_TYPE;
    m_strName.clear();
    m_nSQLType = SQL_TYPE_NULL;
    m_nPrecision = 0;
    m_nScale = 0;
    m_nNullability = 1;

}

FieldInfo::~FieldInfo()
{
}

FieldInfo& FieldInfo::operator = (const FieldInfo& src)
{
    m_nCType = src.m_nCType;
    m_strName = src.m_strName;
    m_nSQLType = src.m_nSQLType;
    m_nPrecision = src.m_nPrecision;
    m_nScale = src.m_nScale;
    m_nNullability = src.m_nNullability;

    return *this;
}

bool FieldInfo::operator == (const FieldInfo& other) const
{
    return 
        m_nCType == other.m_nCType &&
        m_strName == other.m_strName &&
        m_nSQLType == other.m_nSQLType &&
        m_nPrecision == other.m_nPrecision &&
        m_nScale == other.m_nScale &&
        m_nNullability == other.m_nNullability;
}

void FieldInfo::InitData()
{
    m_strName.clear();
    m_nSQLType = 0;
    m_nPrecision = 0;
    m_nScale = 0;
    m_nNullability = SQL_NULLABLE;
    m_nCType = DEFAULT_FIELD_TYPE;
}

short FieldInfo::GetDefaultCType(const FieldInfo& fi)
{
    short nFieldType = DEFAULT_FIELD_TYPE;
    switch (fi.m_nSQLType)
    {
    case SQL_BIT:
        nFieldType = SQL_C_BIT;
        break;

    case SQL_TINYINT:
        nFieldType = SQL_C_UTINYINT;
        break;

    case SQL_SMALLINT:
        nFieldType = SQL_C_SHORT;
        break;

    case SQL_INTEGER:
        nFieldType = SQL_C_LONG;
        break;

    case SQL_REAL:
        nFieldType = SQL_C_FLOAT;
        break;

    case SQL_FLOAT:
    case SQL_DOUBLE:
        nFieldType = SQL_C_DOUBLE;
        break;

    case SQL_DATE: // same as SQL_DATETIME
    case SQL_TIME:
    case SQL_TIMESTAMP:
    case SQL_TYPE_DATE:
    case SQL_TYPE_TIME:
    case SQL_TYPE_TIMESTAMP:
        nFieldType = SQL_C_TIMESTAMP;
        break;

    case SQL_CHAR:
    case SQL_VARCHAR:
    case SQL_LONGVARCHAR:
        nFieldType = SQL_C_CHAR;
        break;

    case SQL_WCHAR:
    case SQL_WVARCHAR:
    case SQL_WLONGVARCHAR:
        nFieldType = SQL_C_WCHAR;
        break;

    case SQL_BINARY:
    case SQL_VARBINARY:
    case SQL_LONGVARBINARY:
        nFieldType = SQL_C_BINARY;
        break;

    case SQL_NUMERIC:
    case SQL_DECIMAL:
        if (fi.m_nScale > 0)
            nFieldType = SQL_C_DOUBLE;
        else if (fi.m_nPrecision <= 10) // does MAXINT fit?
            nFieldType = SQL_C_SLONG;
        else if (fi.m_nPrecision <= 18)
            nFieldType = SQL_C_UBIGINT;
        else
            nFieldType = DEFAULT_FIELD_TYPE;
        break;
    case SQL_BIGINT:
        nFieldType = SQL_C_UBIGINT;
        break;
    case SQL_GUID:
        nFieldType = SQL_C_GUID;
        break;
    case SQL_UNKNOWN_TYPE:
        nFieldType = SQL_C_CHAR;    // a reasonable default for most data types
        break;
    default:
        nFieldType = DEFAULT_FIELD_TYPE;
    }

    return nFieldType;
}

short FieldInfo::GetDefaultCType()
{
    if (m_nCType != DEFAULT_FIELD_TYPE)    // if member explicitly set
        return m_nCType;
    else // otherwise we derive it from m_nSQLType
        return FieldInfo::GetDefaultCType(*this);
}

