#include "dbitem.h"
#include "lvstring.h"
#include <cassert>

using namespace std;
using namespace linguversa;

DBItem::DBItem()
{
    m_nCType = lwvt_null;
    m_pstring = nullptr;
}

DBItem::DBItem( const DBItem& src)
{
    m_nCType = lwvt_null;
    m_pstring = nullptr;
    copyfrom( src);
}

DBItem::~DBItem()
{
    clear();
}

void DBItem::clear()
{
    switch (m_nCType)
    {
    case lwvt_string:
        if (m_pstring)
            delete (tstring*) m_pstring;
        m_pstring = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_astring:
        if (m_pstringw)
            delete (wstring*) m_pstringw;
        m_pstringw = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_wstring:
        if (m_pstringa)
            delete (string*) m_pstringa;
        m_pstringa = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_date:
        if (m_pdate)
            delete (TIMESTAMP_STRUCT*) m_pdate;
        m_pdate = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_bytearray:
        if (m_pByteArray)
            delete (bytearray*) m_pByteArray;
        m_pByteArray = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_uint64:
        if (m_pUInt64)
            delete (ODBCINT64*) m_pUInt64;
        m_pUInt64 = nullptr;
        m_nCType = lwvt_null;
        break;
    case lwvt_guid:
        if (m_pGUID)
            delete (SQLGUID*) m_pGUID;
        m_pGUID = nullptr;
        m_nCType = lwvt_null;
        break;
    default: // The other parts of the union are no pointers.
        // No explicit delete necessary.
        m_lVal = 0L;
        m_nCType = lwvt_null;
    }
}

DBItem& DBItem::operator = ( const DBItem& src)
{
    copyfrom( src);
    return *this;
}

bool DBItem::operator==(const DBItem & other) const
{
    if (m_nCType != other.m_nCType)
        return false;

    switch (m_nCType)
    {
    case lwvt_uchar:
        return m_chVal == other.m_chVal;
    case lwvt_short:
        return m_iVal == other.m_iVal;
    case lwvt_long:
        return m_lVal == other.m_lVal;
    case lwvt_single:
        return m_fltVal == other.m_fltVal;
    case lwvt_double:
        return m_dblVal == other.m_dblVal;
    case lwvt_string:
        return *m_pstring == *other.m_pstring;
    case lwvt_date:
        return
            m_pdate != nullptr && other.m_pdate != nullptr &&
            m_pdate->year == other.m_pdate->year &&
            m_pdate->month == other.m_pdate->month &&
            m_pdate->day == other.m_pdate->day &&
            m_pdate->hour == other.m_pdate->hour &&
            m_pdate->minute == other.m_pdate->minute &&
            m_pdate->second == other.m_pdate->second &&
            m_pdate->fraction == other.m_pdate->fraction;
    case lwvt_bytearray:
        if (m_pByteArray == nullptr || other.m_pByteArray == nullptr || m_pByteArray->size() != other.m_pByteArray->size())
            return false;
        for (unsigned int i = 0; i < m_pByteArray->size(); i++)
        {
            if ((*m_pByteArray)[i] != (*other.m_pByteArray)[i])
                return false;
        }
        return true;
    case lwvt_uint64:
        return
            m_pUInt64 != nullptr && other.m_pUInt64 != nullptr &&
            *m_pUInt64 == *other.m_pUInt64;
    case lwvt_guid:
        // in windows it works this way:
        //     return 
        //m_pGUID != nullptr && other.m_pGUID != nullptr &&
        //*m_pGUID == *other.m_pGUID;

        // but unfortunately we have to implement it explicitly
        if (m_pGUID == nullptr || other.m_pGUID == nullptr ||
            m_pGUID->Data1 != other.m_pGUID->Data1 ||
            m_pGUID->Data2 != other.m_pGUID->Data2 ||
            m_pGUID->Data3 != other.m_pGUID->Data3)
        {
            return false;
        }
        
        for (int i = 0; i < 8; i++)
        {
            if (m_pGUID->Data4[i] != other.m_pGUID->Data4[i])
                return false;
        }
        
        return true;
    default:
        return false;
    }
}

//TODO
tstring DBItem::ConvertToString( const DBItem& var, tstring colFmt)
{
    tstring sValue;
    //bool bNationalDecSep = false;
    tstring DecimalFormat;
    tstring NullDefault;

    // get the Default for NULL value, empty string if not specified
    size_t nPos = colFmt.find( _T("|"));
    if (nPos != tstring::npos)
    {
        NullDefault = (nPos+1 < colFmt.size()) ? colFmt.substr(nPos+1) : _T("");
        colFmt = colFmt.substr(0,nPos);
    }

    // format var according to its type
    switch (var.m_nCType)
    {
    case lwvt_bool:
        if (colFmt.length() == 0)
            colFmt = _T("%0d");
        sValue = linguversa::string_format( colFmt, (unsigned char) var.m_boolVal);
        break;
    case lwvt_uchar:
        if (colFmt.length() == 0)
            colFmt = _T("%0d");
        sValue = linguversa::string_format( colFmt, var.m_chVal);
        break;
    case lwvt_short:
        if (colFmt.length() == 0)
            colFmt = _T("%0d");
        sValue = linguversa::string_format( colFmt, var.m_iVal);
        break;
    case lwvt_long:
        if (colFmt.length() == 0)
            colFmt = _T("%0d");
        sValue = linguversa::string_format( colFmt, var.m_lVal);
        break;
    case lwvt_single:
        DecimalFormat = colFmt;
        //bNationalDecSep = ConvertNationalSeparator( DecimalFormat);
        sValue = linguversa::string_format( DecimalFormat, var.m_fltVal);
        //if (bNationalDecSep)
        //    sValue.Replace(_T("."),_T(","));
        break;
    case lwvt_double: 
        DecimalFormat = colFmt;
        //bNationalDecSep = ConvertNationalSeparator( DecimalFormat);
        sValue = linguversa::string_format( DecimalFormat, var.m_dblVal);
        //if (bNationalDecSep)
        //  sValue.Replace(_T("."),_T(","));
        break;
    case lwvt_date:
        sValue = linguversa::FormatTimeStamp( colFmt, var.m_pdate);
        break;
    case lwvt_string:
        if (var.m_pstring && !colFmt.length() == 0 && colFmt.find(_T('%')) != tstring::npos)
            sValue = string_format(colFmt, var.m_pstring->c_str());
        else
            sValue = var.m_pstring ? *(var.m_pstring) : _T(""); // StrLenFormat(var.m_pstring ? var.m_pstring->c_str() : _T(""), colFmt);
        break;
    #ifdef UNICODE
    case lwvt_wstring:
        {
            wstring str = var.m_pstringw ? *(var.m_pstringw) : L"";
            if (!colFmt.length() == 0 && colFmt.find('%') != wstring::npos)
                sValue = string_format(colFmt, str.c_str());
            else
                sValue = str; //StrLenFormat(str, colFmt);
        }
        break;
    #else
    case lwvt_astring:
        {
            string str = var.m_pstringa ? *(var.m_pstringa) : "";
            if (!colFmt.length() == 0 && colFmt.find('%') != string::npos)
                sValue = string_format(colFmt, str.c_str());
            else
                sValue = str; //StrLenFormat(str, colFmt);
        }
        break;
    #endif

    case lwvt_bytearray:
        if (var.m_pByteArray == nullptr)
            break;
        else
        {
            bytearray& ba = *var.m_pByteArray;
            tstring s;
            sValue = ba.size() ? _T("0x") : _T("");;
            for (unsigned int i = 0; i < ba.size(); i++)
            {
                s = string_format(_T("%02x"), ba[i]);
                sValue += s;
            }
        }
        break;
    case lwvt_uint64:
        if (colFmt.length() == 0)
            colFmt = _T("%0d");
        sValue = string_format( colFmt, *var.m_pUInt64);
        break;
    case lwvt_guid:
        if (var.m_pGUID)
        {
            SQLGUID& g = *var.m_pGUID;
            sValue = string_format( _T("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"), 
                g.Data1, g.Data2, g.Data3, 
                g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
        }
        break;
    case lwvt_null:
        sValue = NullDefault;
        break;
    default:
        sValue = _T("<undefined>");
        break;
    }

    return sValue;
}

void DBItem::copyfrom(const DBItem& src)
{
    if (this == &src)    // if src and target are identical, no need to copy into itself
        return;          // especially avoid the Clear()

    if (m_nCType != src.m_nCType)
    {
        clear();
    }
    
    switch (src.m_nCType)
    {
    case lwvt_null:
        break;
    case lwvt_bool:
        m_nCType = src.m_nCType;
        m_boolVal = src.m_boolVal;
        break;
    case lwvt_uchar:
        m_nCType = src.m_nCType;
        m_chVal = src.m_chVal;
        break;
    case lwvt_short:
        m_nCType = src.m_nCType;
        m_iVal = src.m_iVal;
        break;
    case lwvt_long:
        m_nCType = src.m_nCType;
        m_lVal = src.m_lVal;
        break;
    case lwvt_single:
        m_nCType = src.m_nCType;
        m_fltVal = src.m_fltVal;
        break;
    case lwvt_double: 
        m_nCType = src.m_nCType;
        m_dblVal = src.m_dblVal;
        break;
    case lwvt_string:
        if (m_nCType != src.m_nCType)
        {
            m_pstring = new tstring(*src.m_pstring);
            m_nCType = src.m_nCType;
        }
        else
        {
            *m_pstring = *src.m_pstring;
        }
        break;
    case lwvt_date:
        if (m_nCType != src.m_nCType)
        {
            m_pdate = new TIMESTAMP_STRUCT;
            m_nCType = src.m_nCType;
        }
        m_pdate->year = src.m_pdate ? src.m_pdate->year : 0;
        m_pdate->month = src.m_pdate ? src.m_pdate->month : 0;
        m_pdate->day = src.m_pdate ? src.m_pdate->day : 0;
        m_pdate->hour = src.m_pdate ? src.m_pdate->hour : 0;
        m_pdate->minute = src.m_pdate ? src.m_pdate->minute : 0;
        m_pdate->second = src.m_pdate ? src.m_pdate->second : 0;
        m_pdate->fraction = src.m_pdate ? src.m_pdate->fraction : 0;
        break;
    case lwvt_bytearray:
        if (m_nCType != src.m_nCType)
        {
            m_pByteArray = new bytearray();
            m_nCType = src.m_nCType;
        }
        m_pByteArray->resize(src.m_pByteArray->size());
        for (unsigned int i=0; i < m_pByteArray->size(); i++)
        {
            (*m_pByteArray)[i] = (*src.m_pByteArray)[i];
        }
        break;
    case lwvt_uint64:
        if (m_nCType != src.m_nCType)
        {
            m_pUInt64 = new unsigned ODBCINT64;
            m_nCType = src.m_nCType;
        }
        *m_pUInt64 = *src.m_pUInt64;
        break;
    case lwvt_guid:
        if (m_nCType != src.m_nCType)
        {
            m_pGUID = new SQLGUID;
            m_nCType = src.m_nCType;
        }
        *m_pGUID = *src.m_pGUID;
        break;
    default:
        assert( false);
        clear();
    }
}

std::ostream& linguversa::operator<<(std::ostream& ar, const DBItem& item)
{
    ar << (signed short) item.m_nCType;
    switch (item.m_nCType)
    {
    case DBItem::lwvt_null:
        break;
    case DBItem::lwvt_bool:
        ar << item.m_boolVal;
        break;
    case DBItem::lwvt_uchar:
        ar << item.m_chVal;
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
    //case DBItem::lwvt_wstring:
    //    ar << (item.m_pstringw ? item.m_pstringw->c_str() : L"");
    //    break;
    case DBItem::lwvt_astring:
        ar << (item.m_pstringa ? *item.m_pstringa : (string)"");
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

std::istream& linguversa::operator>>(std::istream& ar, DBItem& var)
{
    signed short n = 0;
    ar >> n;
    DBItem::vartype vt = (DBItem::vartype) n;
    if (vt != var.m_nCType)
        var.clear();

    switch (vt)
    {
    case DBItem::lwvt_null:
        break;
    case DBItem::lwvt_bool:
        ar >> var.m_boolVal;
        break;
    case DBItem::lwvt_uchar:
        ar >> var.m_chVal;
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
        if (var.m_pdate == nullptr || var.m_nCType != vt)
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
        if (var.m_pstring == nullptr || var.m_nCType != vt)
            var.m_pstring = new tstring();
        ar >> (*var.m_pstring);
        break;
    case DBItem::lwvt_wstring:
        //if (var.m_pstringw == nullptr || var.m_nCType != vt)
        //    var.m_pstringw = new wstring();
        //ar >> (*var.m_pstringw);
        break;
    case DBItem::lwvt_astring:
        if (var.m_pstringa == nullptr || var.m_nCType != vt)
            var.m_pstringa = new string();
        ar >> (*var.m_pstringa);
        break;
    case DBItem::lwvt_binary:
        assert(false);
        //TODO ???
        break;
    case DBItem::lwvt_bytearray:
    {
        size_t baSize = 0;
        ar >> baSize;
        if (var.m_pByteArray == nullptr || var.m_nCType != vt)
            var.m_pByteArray = new bytearray();
        bytearray& ba = *var.m_pByteArray;
        ba.resize(baSize);
        for (size_t i = 0; i < ba.size(); i++)
        {
            ar >> ba[i];
        }
    }
    break;
    case DBItem::lwvt_uint64:
        if (var.m_pUInt64 == nullptr || var.m_nCType != vt)
            var.m_pUInt64 = new UINT64();
        ar >> (*var.m_pUInt64);
        break;
    default:
        assert(false);
        // serialize nothing
        break;
    }

    var.m_nCType = vt;

    return ar;
}
