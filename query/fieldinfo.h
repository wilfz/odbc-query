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

#include <sqlext.h>
#include <string>

#define DEFAULT_FIELD_TYPE SQL_TYPE_NULL // pick "C" data type to match SQL data type

using namespace std;

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
        string m_strName;
        SWORD m_nSQLType;
        SQLULEN m_nPrecision;
        SWORD m_nScale;
        SWORD m_nNullability;
    };
}
