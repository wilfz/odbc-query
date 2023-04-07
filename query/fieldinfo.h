#pragma once

#include "tstring.h"

#include <sqlext.h>
#include <iostream>

#define DEFAULT_FIELD_TYPE SQL_TYPE_NULL // pick "C" data type to match SQL data type

namespace linguversa
{
    class FieldInfo
    {
    public:
        FieldInfo();
        ~FieldInfo();

        FieldInfo& operator = (const FieldInfo& src);

        bool operator == (const FieldInfo& other) const;

        void InitData();
        static short GetDefaultCType(const FieldInfo& fi);
        short GetDefaultCType();

        signed short m_nCType;
        // meta data from ODBC
        std::tstring m_strName;
        SWORD m_nSQLType;
        SQLULEN m_nPrecision;
        SWORD m_nScale;
        SWORD m_nNullability;
    };

    std::ostream& operator <<(std::ostream& ar, const FieldInfo& item);
    std::istream& operator >>(std::istream& ar, FieldInfo& item);
}
