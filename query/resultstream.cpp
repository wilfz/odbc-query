#include "resultstream.h"
#include <cassert>

using namespace linguversa;
using namespace std;

std::tostream& linguversa::operator<<(std::tostream& ar, const DBItem& item)
{
    ar << (signed short)item.m_nVarType;
    switch (item.m_nVarType)
    {
    case DBItem::lwvt_null:
        break;
    case DBItem::lwvt_bool:
        ar << item.m_boolVal;
        break;
    case DBItem::lwvt_uchar:
#ifdef UNICODE
        ar << (unsigned short) item.m_chVal;
#else
		ar << item.m_chVal;
#endif
        break;
    case DBItem::lwvt_short:
        ar << item.m_iVal;
        break;
    case DBItem::lwvt_long:
        ar << item.m_lVal;
        break;
    case DBItem::lwvt_single:
        ar << item.m_fltVal;
        break;
    case DBItem::lwvt_double:
        ar << item.m_dblVal;
        break;
    case DBItem::lwvt_date:
        ar << (item.m_pdate ? item.m_pdate->year : (SQLSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->month : (SQLUSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->day : (SQLUSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->hour : (SQLUSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->minute : (SQLUSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->second : (SQLUSMALLINT)0);
        ar << (item.m_pdate ? item.m_pdate->fraction : (SQLUINTEGER)0);
        break;
    case DBItem::lwvt_string:
        ar << (item.m_pstring ? *item.m_pstring : (tstring)_T(""));
        break;
    case DBItem::lwvt_wstring:
        #ifdef UNICODE
        ar << (item.m_pstringw ? *item.m_pstringw : (wstring)L"");
        #endif
        break;
    case DBItem::lwvt_astring:
        #ifndef UNICODE
        ar << (item.m_pstringa ? *item.m_pstringa : (string)"");
        #endif
        break;
    case DBItem::lwvt_binary:
        assert(false);
        //TODO ???
        break;
    case DBItem::lwvt_bytearray:
    {
        bytearray* pByteArray = (bytearray*)item.m_pByteArray;
        if (pByteArray == nullptr)
            pByteArray = new bytearray();
        bytearray& ba = *pByteArray;
        ar << ba.size();
        for (size_t i = 0; i < ba.size(); i++)
        {
            ar << ba[i];
        }
        if (item.m_pByteArray == nullptr)
            delete pByteArray;
    }
    break;
    case DBItem::lwvt_uint64:
        ar << (item.m_pUInt64 ? (*item.m_pUInt64) : (UINT64)0);
        break;
    default:
        assert(false);
        // serialize nothing
        break;
    }

    return ar;
}

std::tistream& linguversa::operator>>(std::tistream& ar, DBItem& var)
{
    signed short n = 0;
    ar >> n;
    DBItem::vartype vt = (DBItem::vartype)n;
    if (vt != var.m_nVarType)
        var.clear();

    switch (vt)
    {
    case DBItem::lwvt_null:
        break;
    case DBItem::lwvt_bool:
        ar >> var.m_boolVal;
        break;
    case DBItem::lwvt_uchar:
#ifdef UNICODE
    {
        unsigned short ch = 0;
        ar >> ch;
        var.m_chVal = (unsigned char) ch;
    }
#else
        ar >> var.m_chVal;
#endif
        break;
    case DBItem::lwvt_short:
        ar >> var.m_iVal;
        break;
    case DBItem::lwvt_long:
        ar >> var.m_lVal;
        break;
    case DBItem::lwvt_single:
        ar >> var.m_fltVal;
        break;
    case DBItem::lwvt_double:
        ar >> var.m_dblVal;
        break;
    case DBItem::lwvt_date:
        if (var.m_pdate == nullptr || var.m_nVarType != vt)
            var.m_pdate = new TIMESTAMP_STRUCT();
        ar >> var.m_pdate->year;
        ar >> var.m_pdate->month;
        ar >> var.m_pdate->day;
        ar >> var.m_pdate->hour;
        ar >> var.m_pdate->minute;
        ar >> var.m_pdate->second;
        ar >> var.m_pdate->fraction;
        break;
    case DBItem::lwvt_string:
        if (var.m_pstring == nullptr || var.m_nVarType != vt)
            var.m_pstring = new tstring();
        ar >> (*var.m_pstring);
        break;
    case DBItem::lwvt_wstring:
        #ifdef UNICODE
        if (var.m_pstringw == nullptr || var.m_nVarType != vt)
            var.m_pstringw = new wstring();
        ar >> (*var.m_pstringw);
        #endif
        break;
    case DBItem::lwvt_astring:
        #ifndef UNICODE
        if (var.m_pstringa == nullptr || var.m_nVarType != vt)
            var.m_pstringa = new string();
        ar >> (*var.m_pstringa);
        #endif
        break;
    case DBItem::lwvt_binary:
        assert(false);
        //TODO ???
        break;
    case DBItem::lwvt_bytearray:
    {
        size_t baSize = 0;
        ar >> baSize;
        if (var.m_pByteArray == nullptr || var.m_nVarType != vt)
            var.m_pByteArray = new bytearray();
        bytearray& ba = *var.m_pByteArray;
        ba.resize(baSize);
        for (size_t i = 0; i < ba.size(); i++)
        {
#ifdef UNICODE
            int ival = 0;
			ar >> ival;
			ba[i] = (unsigned char)ival;
#else
            ar >> ba[i];
#endif
        }
    }
    break;
    case DBItem::lwvt_uint64:
        if (var.m_pUInt64 == nullptr || var.m_nVarType != vt)
            var.m_pUInt64 = new UINT64();
        ar >> (*var.m_pUInt64);
        break;
    default:
        assert(false);
        // serialize nothing
        break;
    }

    var.m_nVarType = vt;

    return ar;
}

std::tostream& linguversa::operator<<(std::tostream& ar, const DataRow& row)
{
    size_t colcount = row.size();
    ar << colcount;
    for (size_t i = 0; i < colcount; i++)
        ar << row[i];
    return ar;
}

std::tistream& linguversa::operator>>(std::tistream& ar, DataRow& row)
{
    size_t colcount = 0;
    ar >> colcount;
    row.resize(colcount);
    for (size_t i = 0; i < colcount; i++)
        ar >> row[i];
    return ar;
}

std::tostream& linguversa::operator<<(std::tostream& ar, const FieldInfo& info)
{
    ar << info.m_nCType << info.m_strName << info.m_nSQLType << info.m_nPrecision << info.m_nScale << info.m_nNullability;
    return ar;
}

std::tistream& linguversa::operator>>(std::tistream& ar, FieldInfo& info)
{
    ar >> info.m_nCType >> info.m_strName >> info.m_nSQLType >> info.m_nPrecision >> info.m_nScale >> info.m_nNullability;
    return ar;
}

std::tostream& linguversa::operator<<(std::tostream& ar, const ResultInfo& resultinfo)
{
    size_t colcount = resultinfo.size();
    ar << colcount;
    for (size_t i = 0; i < colcount; i++)
        ar << resultinfo[i];
    return ar;
}

std::tistream& linguversa::operator>>(std::tistream& ar, ResultInfo& resultinfo)
{
    size_t colcount = 0;
    ar >> colcount;
    resultinfo.resize(colcount);
    for (size_t i = 0; i < colcount; i++)
        ar >> resultinfo[i];
    return ar;
}
