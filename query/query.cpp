#include "query.h"
#include "paramitem.h"
#include <cassert>

using namespace linguversa;
using namespace std;

Query::Query()
{
    m_pConnection = nullptr;
    m_hdbc = SQL_NULL_HDBC;
    InitData();
}

Query::Query( Connection* pConnection)
{
    m_pConnection = nullptr;
    m_hdbc = SQL_NULL_HDBC;
    InitData();
    SetDatabase(pConnection);
}

Query::~Query()
{
    try	// Do not terminate destructor by exception!
    {
        if (m_hstmt)
            Close();
    }
    catch (...) {}
}

void Query::SetDatabase( Connection* pConnection)
{
    if (pConnection != m_pConnection)
    {
        Close();
        m_pConnection = pConnection;
        m_hdbc = m_pConnection ? m_pConnection->GetSqlHDbc() : SQL_NULL_HDBC;
    }
}

void Query::SetDatabase(Connection& connection)
{
    if (&connection != m_pConnection)
    {
        Close();
        m_pConnection = &connection;
        m_hdbc = m_pConnection ? m_pConnection->GetSqlHDbc() : SQL_NULL_HDBC;
    }
}

SQLRETURN Query::ExecDirect( tstring statement)
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls
    if (m_hdbc == NULL && m_pConnection != nullptr)
        m_hdbc = m_pConnection->GetSqlHDbc();
    assert(m_hdbc != SQL_NULL_HDBC);

    if (m_hdbc == NULL)
        return SQL_INVALID_HANDLE;

    m_RowFieldState.clear();

    #ifdef USE_ROWDATA
        m_RowData.clear();
        m_Init.clear();
    #endif

    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_DBC, m_hdbc);
            return nRetCode;
        }
    }

    nRetCode = ::SQLExecDirect( m_hstmt, (SQLTCHAR*) statement.c_str(), SQL_NTS);    // makro sets nRetCode

    // Using ODBC 3 SQLExecDirect/SQLExecute can return SQL_NO_DATA with means SQL_SUCCESS with no rows.
    // From manual:
    //    "If SQLExecDirect executes a searched update or delete statement that
    //    does not affect any rows at the data source, the call to SQLExecDirect
    //    returns SQL_NO_DATA."
    if(!SQL_SUCCEEDED(nRetCode) && nRetCode != SQL_NO_DATA)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);

        return nRetCode;
    }

    RETCODE nRetCode2 = InitFieldInfos();
    if (nRetCode2 != SQL_SUCCESS)
        return nRetCode2;

    return nRetCode;
}

SQLRETURN Query::Close()
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls

    if (m_hstmt == NULL)
        return nRetCode;

    nRetCode = ::SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
    if (!SQL_SUCCEEDED(nRetCode) &&
        // do not throw exception if db was closed earlier than query
        !(nRetCode == SQL_INVALID_HANDLE && (m_pConnection ? m_pConnection->GetSqlHDbc() : m_hdbc) == SQL_NULL_HDBC))
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    InitData();

    return nRetCode;
}

SQLRETURN Query::GetRowsAffected(SQLLEN& nRowCount)
{
    if (m_hstmt == SQL_NULL_HSTMT)
        return SQL_INVALID_HANDLE;

    return ::SQLRowCount(m_hstmt, &nRowCount);
}

SQLRETURN Query::Fetch()
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls

    if (m_hstmt == SQL_NULL_HSTMT)
        return SQL_INVALID_HANDLE;

    int nColCnt = GetODBCFieldCount();
    m_RowFieldState.resize( nColCnt);

    #ifdef USE_ROWDATA
        m_RowData.resize( nColCnt);
        m_Init.resize( nColCnt);
    #endif
    for (int i = 0; i < nColCnt; i++)
    {
        m_RowFieldState[i] = 0;
        #ifdef USE_ROWDATA
            m_RowData[i].clear();
            m_Init[i] = false;
        #endif
    }

    nRetCode = ::SQLFetch( m_hstmt);
    if(!SQL_SUCCEEDED(nRetCode) && nRetCode != SQL_NO_DATA_FOUND)
    {
        throw DbException( nRetCode, SQL_HANDLE_STMT, m_hstmt );
        return nRetCode;
    }
    return nRetCode;
}

RETCODE Query::SQLMoreResults()
{
    RETCODE nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls

    if (m_hstmt == SQL_NULL_HSTMT)
        return SQL_INVALID_HANDLE;

    nRetCode = ::SQLFreeStmt( m_hstmt, SQL_UNBIND);
    if(!SQL_SUCCEEDED(nRetCode))
    {
        throw DbException( nRetCode, SQL_HANDLE_STMT, m_hstmt );
        return nRetCode;
    }

    m_RowFieldState.clear();

#ifdef USE_ROWDATA
    int nColCnt = (int) m_RowData.size();
    for (int i = 0; i < nColCnt; i++)
    {
        m_RowData[i].clear();
        m_Init[i] = false;
    }
    m_RowData.clear();
    m_Init.clear();
#endif

    nRetCode = ::SQLMoreResults( m_hstmt);
    if(!SQL_SUCCEEDED(nRetCode) && nRetCode != SQL_NO_DATA_FOUND)
    {
        throw DbException( nRetCode, SQL_HANDLE_STMT, m_hstmt );
        return nRetCode;
    }

    if (nRetCode == SQL_NO_DATA_FOUND)
        return nRetCode;

    RETCODE nRetCode2 = InitFieldInfos();
    if (!SQL_SUCCEEDED( nRetCode2))
        return nRetCode2;   // TODO

    return nRetCode;
}

// Shutdown pending query for CDatabase's private m_hstmt
void Query::Cancel()
{
    if (m_hstmt != SQL_NULL_HSTMT && SQL_SUCCEEDED(::SQLCancel(m_hstmt)))
        m_hstmt = SQL_NULL_HSTMT;
}

short Query::GetODBCFieldCount() const
{
    SQLSMALLINT colcount = 0;

    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls
    nRetCode = ::SQLNumResultCols( m_hstmt, &colcount);
    if(!SQL_SUCCEEDED(nRetCode))
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    return (short) colcount;
}

int Query::GetFieldIndexByName(tstring lpszFieldName)
{
    if (lpszFieldName.empty())
        return SQL_ERROR;

    int nIndex;
    for (nIndex = 0; nIndex < GetODBCFieldCount(); nIndex++)
    {
        if (m_FieldInfo[nIndex].m_strName == lpszFieldName)
            break;
    }

    // Check if field name found
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        return SQL_ERROR;    // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    return nIndex;
}

void Query::GetODBCFieldInfo(short nIndex, FieldInfo& fieldinfo)
{
    if (nIndex < 0 || (unsigned int) nIndex >= m_FieldInfo.size())
    {
        // TODO:
        assert(false);
        return;
    }

    fieldinfo.m_strName = m_FieldInfo[nIndex].m_strName;
    fieldinfo.m_nSQLType = m_FieldInfo[nIndex].m_nSQLType;
    fieldinfo.m_nPrecision = m_FieldInfo[nIndex].m_nPrecision;
    fieldinfo.m_nScale = m_FieldInfo[nIndex].m_nScale;
    fieldinfo.m_nNullability = m_FieldInfo[nIndex].m_nNullability;
}

bool Query::GetODBCFieldInfo(tstring lpszName, FieldInfo& fieldinfo)
{
    assert(! lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        // TODO:
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    GetODBCFieldInfo(nIndex, fieldinfo);
    return true;
}

void Query::SetODBCFieldInfo(short nIndex, const FieldInfo & fieldinfo)
{
    if (nIndex < 0 || (unsigned int) nIndex >= m_FieldInfo.size())
    {
        assert(false);
        return;
    }

    m_FieldInfo[nIndex].m_strName = fieldinfo.m_strName;
    m_FieldInfo[nIndex].m_nSQLType = fieldinfo.m_nSQLType;
    m_FieldInfo[nIndex].m_nPrecision = fieldinfo.m_nPrecision;
    m_FieldInfo[nIndex].m_nScale = fieldinfo.m_nScale;
    m_FieldInfo[nIndex].m_nNullability = fieldinfo.m_nNullability;
}

bool Query::SetODBCFieldInfo(tstring lpszName, const FieldInfo& fieldinfo)
{
    assert(! lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    SetODBCFieldInfo(nIndex, fieldinfo);
    return true;
}

void Query::SetColumnSqlType(short nIndex, SWORD nSQLType)
{
    if (nIndex < 0 || (unsigned int) nIndex >= m_FieldInfo.size())
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQ_ERROR_FIELD_NOT_FOUND
    m_FieldInfo[nIndex].m_nSQLType = nSQLType;
    return;
}

bool Query::SetColumnSqlType(tstring lpszName, SWORD nSQLType)
{
    int nIndex = GetFieldIndexByName( lpszName);
    if (nIndex < 0)
        return false;
    m_FieldInfo[nIndex].m_nSQLType = nSQLType;
    return true;
}

bool Query::GetFieldValue(tstring lpszName, tstring& sValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
}

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, sValue);
}

bool Query::GetFieldValue(short nIndex, string& sValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException( SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_CHAR);
    #ifndef UNICODE
    if (dbitem.m_nCType == DBItem::lwvt_string)
    {
        sValue = *dbitem.m_pstringa;
        return true;
    }
    #endif

    if (dbitem.m_nCType == DBItem::lwvt_astring)
    {
        sValue = *dbitem.m_pstringa;
        return true;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, wstring& sValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException( SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_WCHAR);
    #ifdef UNICODE
    if (dbitem.m_nCType == DBItem::lwvt_string)
    {
        sValue = *dbitem.m_pstringw;
        return true;
    }
    #endif

    if (dbitem.m_nCType == DBItem::lwvt_wstring)
    {
        sValue = *dbitem.m_pstringw;
        return true;
    }

    return false;
}

bool Query::GetFieldValue(tstring lpszName, long & lValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, lValue);
}

bool Query::GetFieldValue(tstring lpszName, int & iValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, iValue);
}

bool Query::GetFieldValue(tstring lpszName, short & siValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, siValue);
}

bool Query::GetFieldValue(tstring lpszName, bytearray & ba)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, ba);
}

bool Query::GetFieldValue(tstring lpszName, ODBCINT64 & ui64Value)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, ui64Value);
}

bool Query::GetFieldValue(tstring lpszName, double & dValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, dValue);
}

bool Query::GetFieldValue(tstring lpszName, SQLGUID& guid)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue(nIndex, guid);
}

bool linguversa::Query::GetFieldValue(tstring lpszName, TIMESTAMP_STRUCT& tsValue)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        return false;
    }

    // Get the index of the field corresponding to name
    int nIndex = GetFieldIndexByName(lpszName);
    if (nIndex < 0)
        return false;

    return GetFieldValue( nIndex, tsValue);
}

bool Query::GetFieldValue(short nIndex, long & lValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_LONG);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_long:
        lValue = dbitem.m_lVal;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, int & iValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_LONG);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_long:
        iValue = dbitem.m_lVal;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, short & siValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_SHORT);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_short:
        siValue = dbitem.m_iVal;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, bytearray & ba)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_BINARY);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_bytearray:
        ba = *dbitem.m_pByteArray;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, ODBCINT64 & ui64Value)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_UBIGINT);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_bytearray:
        ui64Value = *dbitem.m_pUInt64;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, double & dValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_DOUBLE);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_double:
        dValue = dbitem.m_dblVal;
        return true;
    default:
        return false;
    }

    return false;
}

bool Query::GetFieldValue(short nIndex, SQLGUID& guid)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_GUID);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_guid:
        guid = *dbitem.m_pGUID;
        return true;
    default:
        return false;
    }

    return false;
}

bool linguversa::Query::GetFieldValue(short nIndex, TIMESTAMP_STRUCT& tsValue)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); // AFX_SQL_ERROR_FIELD_NOT_FOUND
    }

    DBItem dbitem;
    GetFieldValue(nIndex, dbitem, SQL_C_TIMESTAMP);
    switch (dbitem.m_nCType)
    {
    case DBItem::lwvt_date:
        tsValue = *dbitem.m_pdate;
        return true;
    default:
        return false;
    }

    return false;
}

void Query::GetFieldValue(short nIndex, DBItem& varValue, short nFieldType)
{
    if (nIndex < 0 || nIndex >= GetODBCFieldCount())
    {
        // TODO:
        throw DbException(SQL_ERROR, SQL_HANDLE_STMT, m_hstmt); /*AFX_SQL_ERROR_FIELD_NOT_FOUND*/
    }

    #ifdef USE_ROWDATA
    if (m_RowData.size() > (unsigned short) nIndex && (m_Init[nIndex] || (m_RowFieldState[nIndex] & 0x01)))	// initialized = already read
    {
        // copy cached value into varValue
        varValue = m_RowData[nIndex];
        return;
    }
    #endif

    // Clear the previous variant
    varValue.clear();

    FieldInfo fieldinfo;
    GetODBCFieldInfo( nIndex, fieldinfo);

    // overriding the field type determination of CRecordset
    if (nFieldType == DEFAULT_FIELD_TYPE)
    {
        nFieldType = FieldInfo::GetDefaultCType( fieldinfo);
    }

    SQLLEN len = 0; //fieldinfo.m_nPrecision;
    SQLRETURN nRetCode = SQL_ERROR;

    switch (nFieldType)
    {
    case SQL_C_BIT:
    {
        SQLCHAR byteValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &byteValue,
            sizeof(byteValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_bool;
            varValue.m_boolVal = (byteValue ? true : false);
        }
    } break;

    case SQL_C_UTINYINT:
    {
        SQLCHAR byteValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &byteValue,
            sizeof(byteValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_uchar;
            varValue.m_chVal = byteValue;
        }
    } break;

    case SQL_C_SHORT:
    {
        short sValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &sValue,
            sizeof(sValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_short;
            varValue.m_iVal = sValue;
        }
    } break;

    case SQL_C_LONG:
    {
        long lValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &lValue,
            sizeof(lValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_long;
            varValue.m_lVal = lValue;
        }
    } break;

    case SQL_C_FLOAT:
    {
        float fValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &fValue,
            sizeof(fValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_single;
            varValue.m_fltVal = fValue;
        }
    } break;

    case SQL_C_DOUBLE:
    {
        double dValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &dValue,
            sizeof(dValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_double;
            varValue.m_dblVal = dValue;
        }
    } break;

    case SQL_C_TIMESTAMP:
    {
        TIMESTAMP_STRUCT tsValue;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &tsValue, sizeof(TIMESTAMP_STRUCT), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_date;
            varValue.m_pdate = new TIMESTAMP_STRUCT;
            *varValue.m_pdate = tsValue;
        }
    } break;

    // special handling of LWVT_BYTEARRAY which is pointer to ByteArray
    case SQL_C_BINARY:
    {
        // Call SQLGetData to determine the amount of data that's waiting.
        BYTE b = 0;    // dummy variable, but necessary!
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &b, 0, &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            BYTE* buf = new BYTE[len + 1];
            // Get all the data at once.
            nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, buf, len + 1, &len);
            if (SQL_SUCCEEDED(nRetCode))
            {
                // initialize varValue with a pointer to bytearray of proper size
                varValue.m_nCType = DBItem::lwvt_bytearray;
                varValue.m_pByteArray = new bytearray();
                bytearray& ba = *(varValue.m_pByteArray);
                ba.resize(len);
                // copy buf to the btearray:
                for (int i = 0; i < len; i++)
                    ba[i] = buf[i];
            }

            delete[] buf;
        }
    } break;

    // special handling of LWVT_UINT64 which is pointer to ODBCINT64
    case SQL_C_UBIGINT:
    {
        ODBCINT64 biValue = 0;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, nFieldType, &biValue, sizeof(biValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_uint64;
            varValue.m_pUInt64 = new unsigned ODBCINT64();
            *varValue.m_pUInt64 = biValue;
        }
    } break;

    // special handling of LWVT_GUID which is pointer to GUID
    case SQL_C_GUID:
    {
        SQLGUID gValue;
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1,
            (fieldinfo.m_nSQLType == SQL_BINARY) ? SQL_C_BINARY : SQL_C_GUID,    //TODO
            &gValue, sizeof(gValue), &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            varValue.m_nCType = DBItem::lwvt_guid;
            varValue.m_pGUID = new SQLGUID;
            *varValue.m_pGUID = gValue;
        }
    } break;

    case SQL_C_WCHAR:
    {
        wchar_t c = 0;    // dummy variable, but necessary!
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_WCHAR, &c, 0, &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            // create a buffer
            size_t n = len / sizeof(wchar_t);
            wchar_t* buf = new wchar_t[n + 1];
            // Get all the data at once.
            nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_WCHAR, buf, (n + 1) * sizeof(wchar_t), &len);
            if (SQL_SUCCEEDED(nRetCode))
            {
                buf[n] = (wchar_t)0;
                // initialize varValue with a pointer to wstring
                #ifdef UNICODE
                varValue.m_nCType = DBItem::lwvt_string;
                #else
                varValue.m_nCType = DBItem::lwvt_wstring;
                #endif
                varValue.m_pstringw = new wstring();
                varValue.m_pstringw->assign(buf);
            }

            delete[] buf;
        }
    } break;
    
    case SQL_C_CHAR:
    {
        char c = 0;    // dummy variable, but necessary!
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_CHAR, &c, 0, &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            // create a buffer
            char* buf = new char[len + 1];
            // Get all the data at once.
            nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_CHAR, buf, len + 1, &len);
            if (SQL_SUCCEEDED(nRetCode))
            {
                buf[len] = (char) 0;
                // initialize varValue with a pointer to string
                #ifdef UNICODE
                varValue.m_nCType = DBItem::lwvt_astring;
                #else
                varValue.m_nCType = DBItem::lwvt_string;
                #endif
                varValue.m_pstringa = new string();
                varValue.m_pstringa->assign(buf);
            }

            delete[] buf;
        }
    } break;

    default:    // most sql types can be converted to tstring
    {
        TCHAR c = 0;    // dummy variable, but necessary!
        nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_TCHAR, &c, 0, &len);
        if (SQL_SUCCEEDED(nRetCode) && len > 0)
        {
            // create a buffer
            size_t n = len/sizeof(TCHAR);
            TCHAR* buf = new TCHAR[n + 1];
            // Get all the data at once.
            nRetCode = ::SQLGetData(m_hstmt, nIndex + 1, SQL_C_TCHAR, buf, (n + 1) * sizeof(TCHAR), &len);
            if (SQL_SUCCEEDED(nRetCode))
            {
                // initialize varValue with a pointer to tstring
                varValue.m_nCType = DBItem::lwvt_string;
                varValue.m_pstring = new tstring();
                varValue.m_pstring->assign(buf);
            }

            delete[] buf;
        }
    } break;

    } // end of case

    if (nRetCode != SQL_SUCCESS)
    {
        SQLTCHAR       SqlState[6], Msg[SQL_MAX_MESSAGE_LENGTH];
        SQLINTEGER    NativeError;
        SQLSMALLINT   i, MsgLen;
        SQLRETURN     rc2;

        i = 1;
        while ((rc2 = ::SQLGetDiagRec(SQL_HANDLE_STMT, m_hstmt, i, SqlState,
            &NativeError, Msg, SQL_MAX_MESSAGE_LENGTH, &MsgLen)) != SQL_NO_DATA)
        {
            //DisplayError(SqlState,NativeError,Msg,MsgLen);
            //TRACE(_T("::SQLGetData() failed: \nSqlState = %s\nNativeError = %d\nErrorText = %s\n"), (LPCSTR)SqlState, NativeError, (LPCSTR)Msg);
            i++;
        }
    }

    if (!SQL_SUCCEEDED(nRetCode))
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);

    // SQL_NULL_DATA is -1: negative len can indicate NULL values
    if (len  < 0)
        m_RowFieldState[nIndex] |= 0x02;    // null value

    m_RowFieldState[nIndex] |= 0x01;    // initialized
#ifdef USE_ROWDATA
    m_RowData[nIndex] = varValue;
    m_Init[nIndex] = true;
#endif

    return;
}

bool Query::GetFieldValue(tstring lpszName, DBItem& varValue, short nFieldType)
{
    assert(! lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        varValue.clear();
        return false;
    }

    // Get the index of the field corresponding to name
    short nField = GetFieldIndexByName(lpszName);
    if (nField < 0)
        return false;

    GetFieldValue(nField, varValue, nFieldType);
    return true;
}

bool Query::GetFieldValue(short nIndex, DBItem& varValue, DBItem::vartype itemtype)
{
    SQLSMALLINT sql_c_typ = SQL_UNKNOWN_TYPE;
    switch (itemtype)
    {
    case DBItem::lwvt_null:
        sql_c_typ = DEFAULT_FIELD_TYPE;
        break;
    case DBItem::lwvt_bool:
        sql_c_typ = SQL_C_BIT;
        break;
    case DBItem::lwvt_uchar:
        sql_c_typ = SQL_C_UTINYINT;
        break;
    case DBItem::lwvt_short:
        sql_c_typ = SQL_C_SHORT;
        break;
    case DBItem::lwvt_long:
        sql_c_typ = SQL_C_LONG;
        break;
    case DBItem::lwvt_single:
        sql_c_typ = SQL_C_FLOAT;
        break;
    case DBItem::lwvt_double:
        sql_c_typ = SQL_C_DOUBLE;
        break;
    case DBItem::lwvt_date:
        sql_c_typ = SQL_C_TIMESTAMP;
        break;
    case DBItem::lwvt_string:
        sql_c_typ = SQL_C_TCHAR;
        break;
    case DBItem::lwvt_binary:
        sql_c_typ = SQL_C_SHORT;
        break;
    case DBItem::lwvt_astring:
        sql_c_typ = SQL_C_CHAR;
        break;
    case DBItem::lwvt_wstring:
        sql_c_typ = SQL_C_WCHAR;
        break;
    case DBItem::lwvt_bytearray:
        sql_c_typ = SQL_C_BINARY;
        break;
    case DBItem::lwvt_uint64:
        sql_c_typ = SQL_C_UBIGINT;
        break;
    case DBItem::lwvt_guid:
        sql_c_typ = SQL_C_GUID;
        break;
    default:
        assert(false);
        return false;
    }

    GetFieldValue(nIndex, varValue, sql_c_typ);
    return true;
}

bool Query::GetFieldValue(tstring lpszName, DBItem& varValue, DBItem::vartype itemtype)
{
    assert(!lpszName.empty());

    // No data or no column info fetched yet
    if (GetODBCFieldCount() <= 0)
    {
        assert(false);
        varValue.clear();
        return false;
    }

    // Get the index of the field corresponding to name
    short nField = GetFieldIndexByName(lpszName);
    if (nField < 0)
        return false;

    return GetFieldValue(nField, varValue, itemtype);
}

bool Query::IsFieldInit( short nIndex)
{
    return (nIndex >= 0 && (unsigned int) nIndex < m_RowFieldState.size() && (m_RowFieldState[nIndex] & 0x01) != 0);    // initialized
}

bool Query::IsFieldInit(tstring lpszName)
{
    // Get the index of the field corresponding to name
    short nField = GetFieldIndexByName(lpszName);
    return IsFieldInit(nField);
}

bool Query::IsFieldNull( short nIndex)
{
    return (nIndex >= 0 && (unsigned int) nIndex < m_RowFieldState.size() && (m_RowFieldState[nIndex] & 0x02) != 0);    // null value
}

bool Query::IsFieldNull(tstring lpszName)
{
    // Get the index of the field corresponding to name
    short nField = GetFieldIndexByName(lpszName);
    return IsFieldNull( nField);
}

RETCODE Query::Prepare(tstring statement)
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls
    if (m_hdbc == NULL && m_pConnection != nullptr)
        m_hdbc = m_pConnection->GetSqlHDbc();

    if (m_hdbc == NULL)
        return SQL_INVALID_HANDLE;  // TODO

    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
        {
            //throw DbException(nRetCode, m_Database, m_hstmt);
            throw DbException(nRetCode, SQL_HANDLE_DBC, m_hdbc);
            return nRetCode;
        }
    }

    nRetCode = ::SQLPrepare( m_hstmt, (SQLTCHAR*)statement.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
    {
        //throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    SQLSMALLINT nParams;
    do {
        nRetCode = ::SQLNumParams(m_hstmt, &nParams);
    } while (nRetCode == SQL_STILL_EXECUTING);

    if (!SQL_SUCCEEDED(nRetCode))
    {
        //throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    /*
    // cleanup and resize m_ParamItem is not necessary because we always
    for (unsigned short i = nParams + 1; i < m_ParamItem.size(); i++)
    {
        ParamItem* pPi = m_ParamItem[i];
        if (pPi)
            delete pPi;
        m_ParamItem[i] = nullptr;
    }
    m_ParamItem.resize(nParams + 1);
    */

    // what about parmeter #0?
    for (SQLUSMALLINT i = 1; i <= nParams; i++)
    {
        ParamItem* pPi = NULL;
        if ((unsigned int)nParams < m_ParamItem.size())
            pPi = (ParamItem*)m_ParamItem[i];
        else
            m_ParamItem.resize(nParams + 1, (ParamItem*) nullptr);

        if (pPi == NULL)
            pPi = new ParamItem();

        SQLSMALLINT paramSqlType = SQL_UNKNOWN_TYPE;
        SQLSMALLINT paramScale = 0, paramNullable = 0;
        SQLULEN paramLen = 0;

        do {
            nRetCode = ::SQLDescribeParam(m_hstmt, i, &paramSqlType, &paramLen, &paramScale, &paramNullable);
        } while (nRetCode == SQL_STILL_EXECUTING);

        if (!SQL_SUCCEEDED(nRetCode))
        {
            //TRACE(_T("Error: ODBC failure getting parameter info.\n"));
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        }

        pPi->m_nSQLType = paramSqlType;
        pPi->m_nPrecision = paramLen;
        pPi->m_nScale = paramScale;
        pPi->m_nParamLen = paramLen;
        pPi->m_nNullability = paramNullable;
        m_ParamItem[i] = pPi;
    }

    m_ParamInitComplete = true;

    return nRetCode;
}

short Query::GetParamCount() const
{
    assert(m_hdbc);
    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
    {
        // TODO
        //throw DbException( (int)SQL_INVALID_HANDLE);
        return SQL_INVALID_HANDLE;
    }

    RETCODE nRetCode = SQL_SUCCESS;	//Return code for ODBC calls
    SQLSMALLINT nParams;
    do {
        nRetCode = ::SQLNumParams(m_hstmt, &nParams);
    } while (nRetCode == SQL_STILL_EXECUTING);

    if (! SQL_SUCCEEDED( nRetCode))
    {
        //AfxThrowDBException(nRetCode, m_pDatabase, m_hstmt);
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
    }

    return (short)nParams;
}

RETCODE Query::GetParamInfo(SQLUSMALLINT ParameterNumber, ParamInfo& paraminfo)
{
    assert(m_hdbc);
    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;  // TODO

    RETCODE nRetCode = SQL_SUCCESS;	//Return code for ODBC calls
    SQLSMALLINT nParams;
    do {
        nRetCode = ::SQLNumParams(m_hstmt, &nParams);
    } while (nRetCode == SQL_STILL_EXECUTING);

    if (! SQL_SUCCEEDED( nRetCode))
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
    }

    if (ParameterNumber > nParams)
        return SQL_ERROR;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(nParams + 1, (ParamItem*) nullptr);

    if (pPi != NULL)
    {
        // No need to get it from ODBC:
        // Either we fetched it before, or it has already been set by the host application.
        paraminfo.m_nSQLType = pPi->m_nSQLType;
        paraminfo.m_nPrecision = pPi->m_nPrecision;
        paraminfo.m_nScale = pPi->m_nScale;
        paraminfo.m_nNullability = pPi->m_nNullability;

        return SQL_SUCCESS;
    }

    SQLSMALLINT paramSqlType;
    SWORD paramScale, paramNullable;
    SQLULEN paramLen = 0;

    do {
        nRetCode = ::SQLDescribeParam(m_hstmt, ParameterNumber, &paramSqlType, &paramLen, &paramScale, &paramNullable);
    } while (nRetCode == SQL_STILL_EXECUTING);

    if (!SQL_SUCCEEDED(nRetCode))
    {
        //TRACE(_T("Error: ODBC failure getting parameter info.\n"));
        //AfxThrowDBException(nRetCode, m_pDatabase, m_hstmt);
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    paraminfo.m_nSQLType = paramSqlType;
    paraminfo.m_nPrecision = paramLen;
    paraminfo.m_nScale = paramScale;
    paraminfo.m_nNullability = paramNullable;
    paraminfo.m_InputOutputType = ParamInfo::input;
    //paraminfo.m_strName.Empty();    // only in stored procedures

    // Cache the ParamInfo, may be the host application will override it later.
    pPi = new ParamItem();

    pPi->m_nSQLType = paramSqlType;
    pPi->m_nPrecision = paramLen;
    pPi->m_nScale = paramScale;
    pPi->m_nParamLen = paramLen;
    pPi->m_nNullability = paramNullable;
    m_ParamItem[ParameterNumber] = pPi;

    return nRetCode;
}

int Query::GetParamIndexByName(tstring ParameterName)
{
    if (ParameterName.empty())
        return -1;

    SQLUSMALLINT n = 0;
    for (n = 1; n < m_ParamItem.size(); n++)
    {
        ParamItem* pPi = m_ParamItem[n];
        if (pPi && pPi->m_ParamName == ParameterName)
            return (short) n;
    }
    
    return -1;
}

SQLRETURN Query::SetParamName(SQLUSMALLINT ParameterNumber, tstring ParameterName)
{
    assert(m_hdbc);
    SQLRETURN nRetCode = SQL_SUCCESS;
 
    ParamItem* pPi = NULL;
    if (m_ParamInitComplete && ParameterNumber >= m_ParamItem.size())
        return SQL_ERROR;
    else if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber + 1, (ParamItem*) nullptr);
 
    if (pPi == NULL)
    {
        pPi = new ParamItem();
        m_ParamItem[ParameterNumber] = pPi;
    }

    // set the name for further calls
    assert(pPi);
    pPi->m_ParamName = ParameterName;
    
    if (m_ParamInitComplete) // Nothing more to do:
        return nRetCode;     // Either we must not bind by name or it has already been done.

    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    // Get ipd handle and set the SQL_DESC_NAMED and SQL_DESC_UNNAMED fields  
    // for record #n.
    SQLHDESC hIpd = SQL_NULL_HDESC;
    nRetCode = ::SQLGetStmtAttr(m_hstmt, SQL_ATTR_IMP_PARAM_DESC, &hIpd, 0, 0);
    if (!SQL_SUCCEEDED(nRetCode) || hIpd == NULL)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    nRetCode = ::SQLSetDescField(hIpd, ParameterNumber, SQL_DESC_NAME, (SQLTCHAR*)ParameterName.c_str(), SQL_NTS);
    if (!SQL_SUCCEEDED(nRetCode))
    {
        throw DbException(nRetCode, SQL_HANDLE_DESC, hIpd);
        return nRetCode;
    }

    return nRetCode;
}

int Query::AddParameter(tstring ParameterName,
    SQLSMALLINT nSqlType,
    UDWORD nLength,
    SWORD nScale,
    SWORD nNullable,
    ParamInfo::InputOutputType inouttype)
{
    if (ParameterName.size() == 0)
        return -1;

    // Does m_ParamItem already include a parameter with the same name?
    int pos = GetParamIndexByName(ParameterName);

    if (pos < 0) // not yet
        pos = (int) m_ParamItem.size(); // append to the end of m_ParamItem

    if (SetParamType((SQLUSMALLINT)pos, nSqlType, nLength, nScale, nNullable, inouttype ) == SQL_SUCCESS)
        return pos;
    else
        return -1;
}

int linguversa::Query::AddParameter(const ParamInfo& paraminfo)
{
    int pos = -1;
    // Does m_ParamItem not yet include a parameter with the same name?
    if (paraminfo.m_ParamName.size() == 0 || (pos = GetParamIndexByName(paraminfo.m_ParamName)) < 0)
        pos = (int) m_ParamItem.size(); // append to the end of m_ParamItem

    if (SetParamType((SQLUSMALLINT)pos, paraminfo) == SQL_SUCCESS)
        return pos;
    else
        return -1;
}

RETCODE Query::SetParamType(SQLUSMALLINT ParameterNumber, SQLSMALLINT nSqlType, UDWORD nLength, SWORD nScale, SWORD nNullable, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;
    ParamItem* pPi = NULL;

    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else if (m_ParamInitComplete)
        return SQL_ERROR;
    else
        m_ParamItem.resize(ParameterNumber + 1, (ParamItem*) nullptr);

    if (pPi == NULL)
    {
        pPi = new ParamItem();
        if (pPi == NULL)
            return SQL_ERROR;
        m_ParamItem[ParameterNumber] = pPi;
    }

    pPi->m_nSQLType = nSqlType;
    pPi->m_nParamLen = pPi->m_nPrecision = nLength;
    if (nScale >= 0)
        pPi->m_nScale = nScale;
    pPi->m_nNullability = nNullable;
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    if (pPi->m_InputOutputType == ParamInfo::unknown)
        pPi->m_InputOutputType = ParamInfo::input;

    return nRetCode;
}

SQLRETURN Query::SetParamType(SQLUSMALLINT ParameterNumber, const ParamInfo& paraminfo)
{
    RETCODE nRetCode = SQL_SUCCESS;
    ParamItem* pPi = NULL;

    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else if (m_ParamInitComplete)
        return SQL_ERROR;
    else
        m_ParamItem.resize(ParameterNumber + 1, (ParamItem*) nullptr);

    if (pPi == NULL)
    {
        pPi = new ParamItem();
        if (pPi == NULL)
            return SQL_ERROR;
        m_ParamItem[ParameterNumber] = pPi;
    }

    pPi->m_nSQLType = paraminfo.m_nSQLType;
    pPi->m_nParamLen = pPi->m_nPrecision = paraminfo.m_nPrecision;
    pPi->m_nScale = paraminfo.m_nScale;
    pPi->m_nNullability = paraminfo.m_nNullability;
    pPi->m_ParamName = paraminfo.m_ParamName;
    pPi->m_InputOutputType = paraminfo.m_InputOutputType;
    if (pPi->m_InputOutputType == ParamInfo::unknown)
        pPi->m_InputOutputType = ParamInfo::input;

    return nRetCode;
}

SQLRETURN Query::Finish()
{
    SQLRETURN nRetCode = SQL_SUCCESS;
    if (GetODBCFieldCount() <= 0)
        return SQL_SUCCESS;

    if (!m_pConnection)
        return SQL_ERROR;

    // find out whether it is "MySQL ODBC ?.? * Driver"
    // otherwise simply return with SQL_SUCCESS
    tstring sDriverName;
	tstring sDriverVersion;
    m_pConnection->SqlGetDriverName(sDriverName);
	m_pConnection->SqlGetDriverVersion(sDriverVersion);

	if (sDriverName.find(_T("maodbc")) != tstring::npos)
	{
		if (sDriverVersion < _T("03.01")) // prevent exception
			return SQL_SUCCESS;
	}

    if (sDriverName.find(_T("myodbc")) == tstring::npos)
    {
        // read behind the last result set and return
        do
        {
            nRetCode = SQLMoreResults();
        } while (SQL_SUCCEEDED(nRetCode));

        if (nRetCode == SQL_NO_DATA_FOUND)
            return SQL_SUCCESS;

        return nRetCode;
    }

    // MySQL's MyODBC driver needs special handling to retrieve output parameters 
    // after the statement has completed:
    // It returns an additional last resultset consisting of the inout and output 
    // parameters in the same order as in the procedure's signature.

    vector< ParamItem*> outparams;
    short nOutput = 0;
    // each inout or output parameter should correspond to exactly one column in the last result set!
    for (unsigned int p = 1; p < m_ParamItem.size(); p++)
    {
        ParamItem* pPi = m_ParamItem[p];
        if (pPi && (pPi->m_InputOutputType == SQL_PARAM_INPUT_OUTPUT ||
            pPi->m_InputOutputType == SQL_PARAM_OUTPUT))
        {
            outparams.resize(nOutput+1);
            outparams[nOutput] = pPi;
            nOutput++;
        }
    }

    if (nOutput == 0)	// no inout or output parameters
        return SQL_SUCCESS;

    // There may be several different result sets.
    // We want to proceed to the last one and retrieve its fieldinfos and first data row.
    vector<FieldInfo> fieldinfo;
    fieldinfo.resize(nOutput);
    vector<DBItem> datarow;
    datarow.resize(nOutput);
    bool singlerow = false;
    nRetCode = SQL_SUCCESS;

    while (nRetCode == SQL_SUCCESS)
    {
        singlerow = false;
        if (GetODBCFieldCount() != nOutput)
        {
            nRetCode = SQLMoreResults();
            continue;
        }

        nRetCode = Fetch();

        // if this is the last resultset there should be exactly one row!
        if (nRetCode == SQL_NO_DATA_FOUND)
        {
            // skip empty resultsets
            nRetCode = SQLMoreResults();
            continue;
        }
        else if (!SQL_SUCCEEDED(nRetCode))
        {
            // something went wrong
            //throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);    // TODO
            return nRetCode;
        }

        for (short col = 0; col < nOutput; col++)
        {
            GetODBCFieldInfo(col, fieldinfo[col]);
            FieldInfo& fi = fieldinfo[col];
            short fieldtype = fi.GetDefaultCType();
            GetFieldValue(col, datarow[col], fieldtype);
        }

        // If this is the last result set
        // there must be no more rows to be fetched!
        if (Fetch() == SQL_NO_DATA_FOUND)
            singlerow = true;
        else
            singlerow = false;

        // yet another result set?
        nRetCode = SQLMoreResults();
        if (GetODBCFieldCount() == 0)
            break;
    }

    // Now we should have the one and only datarow and fieldinfo of the very last result set.
    if (singlerow == false)
        return SQL_ERROR;

    // Copy the values from the one and only row of the very last result set
    // into the bound parameter buffers.
    for (unsigned int i = 0; i < outparams.size(); i++)
    {
        ParamItem* po = outparams[i];
        if (po == nullptr)
            return SQL_ERROR;

        if (datarow[i].m_nCType == DBItem::lwvt_null)
        {
            po->m_lenInd = SQL_NULL_DATA;
            continue;
        }

        switch (po->m_nCType)
        {
        case SQL_C_TCHAR:
            if (po->m_pParam && datarow[i].m_nCType == DBItem::lwvt_string && po->m_nParamLen > 0)
            {
                TCHAR* buf = (TCHAR*)po->m_pParam;
                if (buf == nullptr || datarow[i].m_nCType != DBItem::lwvt_string)
                    return SQL_ERROR;
                // if the target buffer is not yet large enough
                if (po->m_nParamLen + 1 < (int)datarow[i].m_pstring->size() && po->m_local)
                {
                    // locally we can simply allocate a larger buffer
                    delete[]((TCHAR*)buf);
                    po->m_nParamLen = datarow[i].m_pstring->size();
                    po->m_pParam = buf = new TCHAR[po->m_nParamLen + 1];
                }

                // if po->m_pParam points to a host variable outside of Query (external binding)
                // we possibly have to truncate the tstring to fit into the buffer!
                // copy tstring from datarow's field into the output parameter's buffer
                for (int j = 0; j < po->m_nParamLen; j++)
                {
                    if (j < (int) datarow[i].m_pstring->size())
                        buf[j] = (*datarow[i].m_pstring)[j];
                    else
                        buf[j] = (TCHAR)0x00;
                }
                buf[po->m_nParamLen] = (TCHAR)0x00;
            }
            else
            {
                return SQL_ERROR;
            }
            break;

        case SQL_C_LONG:
            if (po->m_pParam == nullptr)
                return SQL_ERROR;
            if (datarow[i].m_nCType == DBItem::lwvt_long)
                *(long*)po->m_pParam = datarow[i].m_lVal;
            else
                return SQL_ERROR;

            break;

         // TODO: ...
        default:
            return SQL_ERROR;
        }
    }

    return SQL_SUCCESS;
}


RETCODE Query::BindParameter(	// 16 bit, SQL_SMALLINT
    SQLUSMALLINT ParameterNumber,
    short& nParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = DBItem::lwvt_short;
    pPi->m_nCType = SQL_C_SHORT;
    pPi->m_pParam = &nParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_SHORT,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_SMALLINT,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &nParamRef,
        (SQLINTEGER)0,		// BufferLength argument is ignored
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(	// 32 bit, SQL_INTEGER
    SQLUSMALLINT ParameterNumber,
    int& nParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = LwDBVariant::lwvt_long;
    pPi->m_nCType = SQL_C_LONG;
    pPi->m_pParam = &nParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_LONG,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_INTEGER,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &nParamRef,
        (SQLINTEGER)0,		// BufferLength argument is ignored
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(tstring ParameterName, int& nParamRef, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;
    int n = GetParamIndexByName(ParameterName);
    if (n < 0)
        return SQL_ERROR;
    
    // Populate record #n of ipd.  
    nRetCode = BindParameter(n, nParamRef, inouttype);
    if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    nRetCode = SetParamName( n, ParameterName);
    return nRetCode;
}

RETCODE Query::BindParameter(	// 32 bit, SQL_INTEGER
    SQLUSMALLINT ParameterNumber,
    long& nParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = LwDBVariant::lwvt_long;
    pPi->m_nCType = SQL_C_LONG;
    pPi->m_pParam = &nParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown) // explicitly specified by the caller
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown) // previously specified
        inouttype = pPi->m_InputOutputType;
    else // nothing is specified => set to ParamInfo::input
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_LONG,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_INTEGER,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &nParamRef,
        (SQLINTEGER)0,		// BufferLength argument is ignored
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(tstring ParameterName, long& nParamRef, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;
    int n = GetParamIndexByName(ParameterName);
    if (n < 0)
        return SQL_ERROR;

    // Populate record #n of ipd.  
    nRetCode = BindParameter(n, nParamRef, inouttype);
    if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    nRetCode = SetParamName(n, ParameterName);
    return nRetCode;
}

RETCODE Query::BindParameter(	// 64 bit, SQL_BIGINT
    SQLUSMALLINT ParameterNumber,
    ODBCINT64& nParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = LwDBVariant::lwvt_uint64;
    pPi->m_nCType = SQL_C_UBIGINT;
    pPi->m_pParam = &nParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    // maybe it is necessary to set pPi->m_lenInd to SQL_NTS (null terminted tstring) or the BufferLength argument to 8 (64 bit is 8 byte)
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_SBIGINT,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_BIGINT,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &nParamRef,
        (SQLINTEGER)0,		// BufferLength argument is ignored
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(
    SQLUSMALLINT ParameterNumber,
    double& dParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = LwDBVariant::lwvt_double;
    pPi->m_nCType = SQL_C_DOUBLE;
    pPi->m_pParam = &dParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_DOUBLE,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_DOUBLE,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &dParamRef,
        (SQLINTEGER)0,		// BufferLength argument is ignored
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(
    SQLUSMALLINT ParameterNumber,
    TIMESTAMP_STRUCT& tsParamRef,
    ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable before binding a new buffer;
    if (pPi == NULL)
        pPi = new ParamItem();
    //pPi->m_dwType = LwDBVariant::lwvt_date;
    pPi->m_nCType = SQL_C_TIMESTAMP;
    pPi->m_pParam = &tsParamRef;
    pPi->m_lenInd = 0;	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        SQL_C_TIMESTAMP,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_TIMESTAMP,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &tsParamRef,
        (SQLINTEGER) sizeof(TIMESTAMP_STRUCT),		// BufferLength argument
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(SQLUSMALLINT ParameterNumber, SQLGUID& guid, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi == NULL)
        pPi = new ParamItem();

    pPi->m_nCType = SQL_C_GUID;
    pPi->m_pParam = &guid;
    pPi->m_lenInd = sizeof(SQLGUID);	// will receive the size of the parameter for inouttype SQL_PARAM_OUTPUT or SQL_PARAM_INPUT_OUTPUT
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        (pPi->m_nSQLType == SQL_BINARY) ? SQL_C_BINARY : pPi->m_nCType,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_GUID,
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        &guid,
        (SQLINTEGER) sizeof(SQLGUID),		// BufferLength argument
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(SQLUSMALLINT ParameterNumber, TCHAR* bufParamRef, SQLLEN bufParamlen, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi == NULL)
        pPi = new ParamItem();

    // if we do not know m_nParamLen from the prepare statement the caller must provide its size with the buffers length
    if (pPi->m_nParamLen == 0 && bufParamlen > 1)
        pPi->m_nParamLen = bufParamlen - 1;

    //pPi->m_dwType = LWVT_TCHARBUFFER;
    pPi->m_nCType = SQL_C_TCHAR;
    pPi->m_pParam = bufParamRef;
    pPi->m_lenInd = SQL_NTS;	// null terminted tstring
    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
#ifdef _UNICODE
        SQL_C_WCHAR,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_WVARCHAR,
#else
        SQL_C_CHAR,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_VARCHAR,
#endif
        //SQL_CHAR,	SQL_VARCHAR, SQL_LONGVARCHAR, LONG VARCHAR, SQL_WCHAR, WCHAR, SQL_WVARCHAR, VARWCHAR, SQL_WLONGVARCHAR
        (SQLLEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        bufParamRef,	// no extra for the terminating 0
        (SQLINTEGER)(bufParamlen) * sizeof(TCHAR),
        &(pPi->m_lenInd));

    return nRetCode;
}

RETCODE Query::BindParameter(tstring ParameterName, TCHAR* bufParamRef, SQLLEN bufParamlen, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;
    int n = GetParamIndexByName(ParameterName);
    if (n < 0)
        return SQL_ERROR;

    // Populate record #n of ipd.  
    nRetCode = BindParameter(n, bufParamRef, bufParamlen, inouttype);
    if (!SQL_SUCCEEDED(nRetCode) || m_hstmt == NULL)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    nRetCode = SetParamName(n, ParameterName);
    return nRetCode;
}

RETCODE Query::BindParameter(SQLUSMALLINT ParameterNumber, BYTE* pBa, SQLLEN paramDataLen, SQLLEN paramBufLen, ParamInfo::InputOutputType inouttype)
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    if (m_hstmt == NULL)
    {
        // Allocate new Statement Handle based on existing connection
        nRetCode = ::SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc, &m_hstmt);
        if (! SQL_SUCCEEDED( nRetCode) || m_hstmt == NULL)
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
            return nRetCode;
        }
    }

    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    else
        m_ParamItem.resize(ParameterNumber+1, (ParamItem*) nullptr);
    if (pPi == NULL)
        pPi = new ParamItem();

    //pPi->m_dwType = LWVT_TCHARBUFFER;
    pPi->m_nCType = SQL_C_BINARY;
    pPi->m_pParam = pBa;
    pPi->m_lenInd = paramDataLen;	// length of valid data in bytes or special marker for NULL data
    if (pPi->m_lenInd < 0 && pPi->m_lenInd != SQL_NULL_DATA)
        pPi->m_lenInd = 0;

    if (paramBufLen == 0 && pPi->m_lenInd > 0)
        paramBufLen = pPi->m_lenInd;

    // if we do not know m_nParamLen from the prepare statement the caller must provide its size with the buffers length
    if (pPi->m_nParamLen <= 0 && paramBufLen > 0)
        pPi->m_nParamLen = paramBufLen;

    pPi->m_local = false;
    // inouttype must be different from ParamInfo::unknown!
    if (inouttype != ParamInfo::unknown)
        pPi->m_InputOutputType = inouttype;
    else if (pPi->m_InputOutputType != ParamInfo::unknown)
        inouttype = pPi->m_InputOutputType;
    else
        inouttype = pPi->m_InputOutputType = ParamInfo::input;
    m_ParamItem[ParameterNumber] = pPi;

    nRetCode = ::SQLBindParameter(m_hstmt, ParameterNumber, (SQLSMALLINT) inouttype,
        pPi->m_nCType,
        pPi->m_nSQLType ? pPi->m_nSQLType : SQL_BINARY,
        (SQLULEN)pPi->m_nParamLen,			// ColumnSize argument
        (SQLSMALLINT)(pPi->m_nScale >= 0 ? pPi->m_nScale : 0),	// DecimalDigits argument
        pBa,	// no extra since there is no terminating 0
        (SQLLEN)(paramBufLen),	// allocated buffer size (must stay valid for complete life cycle of the statement)
        &(pPi->m_lenInd));	// real data length (can change on execution but must never be larger than paramBufLen)

    return nRetCode;
}

SQLLEN Query::GetParamDataLength(SQLUSMALLINT ParameterNumber)
{
    assert(m_hdbc);
    assert(m_hstmt);

    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size() && (pPi = m_ParamItem[ParameterNumber]) != NULL)
    {
        return pPi->m_lenInd;
    }
    else
    {
        throw DbException(SQL_PARAM_ERROR, SQL_HANDLE_STMT, m_hstmt);	// TODO
        return SQL_NULL_DATA;
    }
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, long nParamValue, ParamInfo::InputOutputType inouttype)
{
    ParamItem* pPi = NULL;
    if (m_ParamInitComplete)
        assert(ParameterNumber < m_ParamItem.size());
    else if (ParameterNumber >= m_ParamItem.size())
        m_ParamItem.resize(ParameterNumber + 1);

    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_LONG && pPi->m_pParam) // already bound to local heap variable of type long
    {
        assert(pPi->m_pParam != NULL);
        // so we only have to assign the new value:
        *((long*)(pPi->m_pParam)) = nParamValue;
        return SQL_SUCCESS;
    }

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    long* pLong = new long(nParamValue);	// create new local heap variable

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, *pLong, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, double dParamValue, ParamInfo::InputOutputType inouttype)
{
    ParamItem* pPi = NULL;
    assert(ParameterNumber < m_ParamItem.size());
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_DOUBLE && pPi->m_pParam) // already bound to local heap variable of type double
    {
        assert(pPi->m_pParam != NULL);
        // so we only have to assign the new value:
        *((double*)(pPi->m_pParam)) = dParamValue;
        return SQL_SUCCESS;
    }

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    double* pDouble = new double(dParamValue);	// create new local heap variable

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, *pDouble, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, TIMESTAMP_STRUCT tsParamValue, ParamInfo::InputOutputType inouttype)
{
    ParamItem* pPi = NULL;
    assert(ParameterNumber < m_ParamItem.size());
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_TIMESTAMP && pPi->m_pParam) // already bound to local heap variable of type TIMESTAMP_STRUCT
    {
        // so we only have to assign the new value:
        *((TIMESTAMP_STRUCT*)(pPi->m_pParam)) = tsParamValue;
        return SQL_SUCCESS;
    }

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    TIMESTAMP_STRUCT* pTS = new TIMESTAMP_STRUCT(tsParamValue);	// create new local heap variable

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, *pTS, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, SQLGUID guid, ParamInfo::InputOutputType inouttype)
{
    ParamItem* pPi = NULL;
    assert(ParameterNumber < m_ParamItem.size());
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_GUID && pPi->m_pParam) // already bound to local heap variable of type TIMESTAMP_STRUCT
    {
        // so we only have to assign the new value:
        *((SQLGUID*)(pPi->m_pParam)) = guid;
        return SQL_SUCCESS;
    }

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    SQLGUID* pGuid = new SQLGUID(guid);	// create new local heap variable

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, *pGuid, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, tstring lpszValue, ParamInfo::InputOutputType inouttype, SQLLEN fieldlen)
{
    ParamItem* pPi = NULL;
    assert(ParameterNumber < m_ParamItem.size());
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    // pPi->m_nParamLen and fieldlen do not include one extra char for the tstring terminator.
    if (pPi && fieldlen == 0)
        fieldlen = pPi->m_nParamLen;
    else if (pPi && pPi->m_nParamLen == 0)
        pPi->m_nParamLen = fieldlen;

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_TCHAR && pPi->m_pParam) 	// already bound to local heap variable of type CString
    {
        if (pPi->m_nParamLen > 1 && (long) lpszValue.length() < pPi->m_nParamLen)
        {
            // so we only have to copy the new tstring to the (same old) buffer:
            for (unsigned long i = 0; i < lpszValue.length(); i++)
                ((TCHAR*)(pPi->m_pParam))[i] = lpszValue[i];
            // terminating 0x00 and fill up the remaining buffer:
            for (unsigned long i = (unsigned long) lpszValue.length(); i < (unsigned long) (pPi->m_nParamLen) + 1; i++)    // +1 because of terminating 0x00
                ((TCHAR*)(pPi->m_pParam))[i] = (TCHAR)0x00;
            return SQL_SUCCESS;
        }
    }

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    // create new local buffer on the heap
    TCHAR* pBuf = new TCHAR[fieldlen + 1];    // +1 because of terminating 0x00
    //_tcsncpy_s( (TCHAR*)(pBuf), fieldlen + 1, lpszValue, _TRUNCATE);
    for (int i = 0; i < fieldlen; i++)
    {
        if (i < (int)lpszValue.length())
            pBuf[i] = lpszValue[i];
        else
            pBuf[i] = (TCHAR)0x00;
    }
    // set the last allocated TCHAR of the buffer with terminating 0x00
    pBuf[fieldlen] = (TCHAR)0x00;

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, pBuf, fieldlen + 1, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamValue(SQLUSMALLINT ParameterNumber, const bytearray& ba, ParamInfo::InputOutputType inouttype, SQLLEN fieldlen)
{
    ParamItem* pPi = NULL;
    assert(ParameterNumber < m_ParamItem.size());
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];

    if (pPi && pPi->m_local && pPi->m_nCType == SQL_C_BINARY && pPi->m_pParam && pPi->m_nParamLen > 0 	// already bound to local heap variable of type CString
        && (fieldlen == 0 || fieldlen == pPi->m_nParamLen))
    {
        assert(pPi->m_pParam != NULL);
        // so we only have to copy the current ByteaArray to the (same old) buffer:
        for (unsigned int i = 0; i < ba.size(); i++)
            ((BYTE*)(pPi->m_pParam))[i] = ba[i];

        return SQL_SUCCESS;
    }

    if (pPi && fieldlen <= 0)
        fieldlen = pPi->m_nParamLen;

    if (fieldlen <= 0)
        fieldlen = ba.size();

    if (pPi && pPi->m_local)
        pPi->Clear();	// delete local heap variable;

    // create new local buffer on the heap
    BYTE* pBuf = new BYTE[fieldlen];
    int nLen = (int) (((int) ba.size() < fieldlen) ? ba.size() : fieldlen);
    for (int i = 0; i < nLen; i++)
        pBuf[i] = ba[i];

    // Don't care if it was previously bound from outside: created outside -> deletion outside.
    // We simply overwrite our pointer ...
    RETCODE nRetCode = BindParameter(ParameterNumber, pBuf, nLen, fieldlen, inouttype);
    assert(nRetCode == SQL_SUCCESS);
    pPi = m_ParamItem[ParameterNumber];
    assert(pPi && pPi->m_pParam);
    // ... and take care that the marker for local creation is set:
    pPi->m_local = true;

    return nRetCode;
}

RETCODE Query::SetParamNull(SQLUSMALLINT ParameterNumber)
{
    if (ParameterNumber < 0 || ParameterNumber >= m_ParamItem.size())
        return SQL_ERROR;

    ParamItem* pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL)
    {
        pPi = new ParamItem();
        m_ParamItem[ParameterNumber] = pPi;
    }

    pPi-> m_lenInd = SQL_NULL_DATA;
    return SQL_SUCCESS;
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, long& nParamValue)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL)
        return false;

    switch (pPi->m_nCType)
    {
    case SQL_C_UTINYINT:
        nParamValue = *((BYTE*)(pPi->m_pParam));
        return true;
    case SQL_C_LONG:
        nParamValue = *((long*)(pPi->m_pParam));
        return true;
    case SQL_C_SHORT:
        nParamValue = *((short*)(pPi->m_pParam));
        return true;
    default:
        return false;
    }
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, double& dParamValue)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL)
        return false;

    switch (pPi->m_nCType)
    {
    case SQL_C_FLOAT:
        dParamValue = *((float*)(pPi->m_pParam));
        return true;
    case SQL_C_DOUBLE:
        dParamValue = *((double*)(pPi->m_pParam));
        return true;
    default:
        return false;
    }
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, TIMESTAMP_STRUCT& tsParamValue)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL)
        return false;

    switch (pPi->m_nCType)
    {
    case SQL_C_TIMESTAMP:
    {
        TIMESTAMP_STRUCT& ts = *((TIMESTAMP_STRUCT*)(pPi->m_pParam));
        tsParamValue.year = ts.year;
        tsParamValue.month = ts.month;
        tsParamValue.day = ts.day;
        tsParamValue.hour = ts.hour;
        tsParamValue.minute = ts.minute;
        tsParamValue.second = ts.second;
        tsParamValue.fraction = ts.fraction;
    }
    return true;
    default:
        return false;
    }
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, tstring& sParamValue)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL)
        return false;

    switch (pPi->m_nCType)
    {
    case SQL_C_TCHAR:
        sParamValue = (TCHAR*)(pPi->m_pParam);
        return true;
    default:
        return false;
    }
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, SQLGUID& guid)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL || pPi->m_nCType != SQL_C_GUID)
        return false;

    switch (pPi->m_nSQLType)
    {
    case SQL_GUID:
        guid = *((SQLGUID*)(pPi->m_pParam));
        return true;
    case SQL_BINARY:
        if (pPi->m_lenInd != sizeof(SQLGUID))
            return false;
        guid = *((SQLGUID*)(pPi->m_pParam));
    default:
        return false;
    }
}

bool Query::GetParamValue(SQLUSMALLINT ParameterNumber, bytearray& ba)
{
    ParamItem* pPi = NULL;
    if (ParameterNumber < m_ParamItem.size())
        pPi = m_ParamItem[ParameterNumber];
    if (pPi == NULL || pPi->m_pParam == NULL)
        return false;

    SQLLEN datalen = pPi->m_lenInd;
    assert(datalen <= pPi->m_nParamLen);
    if (datalen > pPi->m_nParamLen)
        datalen = pPi->m_nParamLen;
    if (datalen < 0)
    {
        datalen = 0;
        ba.clear();
        return true;
    }

    switch (pPi->m_nCType)
    {
    case SQL_C_BINARY:
        ba.resize(datalen);
        //memcpy(ba.GetData(), (BYTE*)(pPi->m_pParam), datalen);
        for (int i = 0; i < datalen; i++)
            ba[i] = ((BYTE*)(pPi->m_pParam))[i];
        return true;
    default:
        return false;
    }
}
void Query::SetParamLenIndicator(SQLUSMALLINT ParameterNumber, SQLLEN lenInd)
{
    if (ParameterNumber < m_ParamItem.size() && m_ParamItem[ParameterNumber] != NULL)
    {
        ParamItem* pPi = m_ParamItem[ParameterNumber];
        pPi->m_lenInd = lenInd;
    }
    else
    {
        throw DbException(SQL_PARAM_ERROR, SQL_HANDLE_STMT, m_hstmt);
    }
}

RETCODE Query::Execute()
{
    RETCODE nRetCode = SQL_SUCCESS;	//Return code for your ODBC calls
    assert(m_hdbc);
    assert(m_hstmt);
    if (m_hdbc == NULL || m_hstmt == NULL)
        return SQL_INVALID_HANDLE;

    nRetCode = ::SQLExecute(m_hstmt);

    // Using ODBC 3 SQLExecDirect/SQLExecute can return SQL_NO_DATA with means SQL_SUCCESS with no rows.
    // From manual:
    //	"If SQLExecDirect executes a searched update or delete statement that
    //	does not affect any rows at the data source, the call to SQLExecDirect
    //	returns SQL_NO_DATA."
    if (! SQL_SUCCEEDED( nRetCode) && nRetCode != SQL_NO_DATA)
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        return nRetCode;
    }

    RETCODE nRetCode2 = InitFieldInfos();
    if (nRetCode2 != SQL_SUCCESS)
        return nRetCode2;

    return nRetCode;
}

//END


void Query::InitData()
{
    m_hstmt = NULL;

    for (unsigned int i = 0; i < m_ParamItem.size(); i++)
    {
        ParamItem* pPi = (ParamItem*) m_ParamItem[i];
        if (pPi != nullptr)
            delete pPi;
        m_ParamItem[i] = nullptr;
    }
    m_ParamItem.clear();
    m_ParamInitComplete = false;

    m_FieldInfo.clear();
    m_RowFieldState.clear();
#ifdef USE_ROWDATA
    m_RowData.clear();
    m_Init.clear();
#endif
}

RETCODE Query::InitFieldInfos()
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls
    assert(m_hstmt != SQL_NULL_HSTMT);

    if (m_hstmt == SQL_NULL_HSTMT)
        return SQL_INVALID_HANDLE;  // TODO

    short nFieldCount = GetODBCFieldCount();
    assert(nFieldCount >= 0);
    if (nFieldCount <= 0)
    {
        m_FieldInfo.clear();
        return nRetCode;
    }

    m_FieldInfo.resize(nFieldCount);
    for (SQLSMALLINT col = 0; col < nFieldCount; col++)
    {
        const short BufferLength = 255;
        SQLTCHAR colname[BufferLength + 1];
        SQLSMALLINT namelength = 0;
        SQLSMALLINT datatype = 0;
        SQLULEN columnsize = 0;
        SQLSMALLINT decimaldigits = 0;
        SQLSMALLINT nullable = 0;

        nRetCode = ::SQLDescribeCol(
            m_hstmt,
            col + 1, colname, BufferLength, &namelength,
            &datatype, &columnsize,
            &decimaldigits, &nullable);

        if (!SQL_SUCCEEDED(nRetCode))
        {
            throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);

            return nRetCode;
        }

        assert(namelength <= BufferLength);
        //CString m_strName;
        //SWORD m_nSQLType;
        //UDWORD m_nPrecision;
        //SWORD m_nScale;
        //SWORD m_nNullability;
        m_FieldInfo[col].m_strName.assign( (const TCHAR*) colname);
        m_FieldInfo[col].m_nSQLType = datatype;
        m_FieldInfo[col].m_nPrecision = columnsize;
        m_FieldInfo[col].m_nScale = decimaldigits;
        m_FieldInfo[col].m_nNullability = nullable;
    }

    return nRetCode;
}
