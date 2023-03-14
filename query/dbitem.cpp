#include "dbitem.h"
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

// TODO
tstring DBItem::ConvertToString( const DBItem& var, tstring colFmt)
{
	tstring sValue;
	//bool bNationalDecSep = false;
	tstring DecimalFormat;
	tstring NullDefault;
/*
	int nPos = colFmt.Find( _T("|"));
	if (nPos >= 0)
	{
		NullDefault = (nPos+1 < colFmt.GetLength()) ? colFmt.Mid(nPos+1) : _T("");
		colFmt = colFmt.Left(nPos);
	}
*/
	switch (var.m_nCType)
	{
/*
	case DBVT_BOOL:
		if (colFmt.IsEmpty())
			colFmt = _T("%0d");
		sValue.Format( colFmt, var.m_boolVal);
		break;
	case DBVT_UCHAR:
		if (colFmt.IsEmpty())
			colFmt = _T("%0d");
		sValue.Format( colFmt, var.m_chVal);
		break;
	case DBVT_SHORT:
		if (colFmt.IsEmpty())
			colFmt = _T("%0d");
		sValue.Format( colFmt, var.m_iVal);
		break;
	case DBVT_LONG:
		if (colFmt.IsEmpty())
			colFmt = _T("%0d");
		sValue.Format( colFmt, var.m_lVal);
		break;
	case DBVT_SINGLE:
		DecimalFormat = colFmt;
		bNationalDecSep = ConvertNationalSeparator( DecimalFormat);
		sValue.Format( DecimalFormat, var.m_fltVal);
		if (bNationalDecSep)
			sValue.Replace(_T("."),_T(","));
		break;
	case DBVT_DOUBLE: 
		DecimalFormat = colFmt;
		bNationalDecSep = ConvertNationalSeparator( DecimalFormat);
		sValue.Format( DecimalFormat, var.m_dblVal);
		if (bNationalDecSep)
			sValue.Replace(_T("."),_T(","));
		break;
	case DBVT_DATE:
		sValue = FormatTimeStamp( colFmt, var.m_pdate);
		break;
	case DBVT_STRING:
		if (var.m_pstring && !colFmt.IsEmpty() && colFmt.Find(_T('%'))>=0)
			sValue.Format( colFmt, (LPCTSTR) *(var.m_pstring));
		else
			sValue = StrLenFormat( var.m_pstring ? (LPCTSTR) *(var.m_pstring) : _T(""), colFmt);
		break;
	#if _MFC_VER >= 0x0700
	case DBVT_ASTRING:
		{
			CString str = var.m_pstringA ? CString((LPCSTR) *(var.m_pstringA)) : _T("");
			if (!colFmt.IsEmpty() && colFmt.Find(_T('%'))>=0)
				sValue.Format( colFmt, (LPCTSTR) str);
			else
				sValue = StrLenFormat( str, colFmt);
		}
		break;
	case DBVT_WSTRING:
		{
			CString str = var.m_pstringW ? (LPCWSTR) *(var.m_pstringW) : L"";
			if (!colFmt.IsEmpty() && colFmt.Find(_T('%'))>=0)
				sValue.Format( colFmt, (LPCTSTR) str);
			else
				sValue = StrLenFormat( str, colFmt);
		}
		break;
	#endif
	case LWVT_BYTEARRAY:
		if (var.m_pbinary == NULL)
			break;
		else
		{
			CByteArray& ba = *(CByteArray*) var.m_pbinary;
			CString s;
			sValue = ba.GetSize() ? _T("0x") : _T("");;
			for (int i = 0; i < ba.GetSize(); i++)
			{
				s.Format(_T("%02x"), ba[i]);
				sValue += s;
			}
		}
		break;
	case LWVT_UINT64:
		if (colFmt.IsEmpty())
			colFmt = _T("%0d");
		sValue.Format( colFmt, *(UINT64*) var.m_pbinary);
		break;
	case LWVT_GUID:
		{
			LwString s(*(GUID*)var.m_pbinary);
			sValue = s;
		}
		break;
*/
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
    //case lwvt_bool:
    //    m_nCType = src.m_nCType;
    //    m_boolVal = src.m_boolVal;
    //    break;
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
