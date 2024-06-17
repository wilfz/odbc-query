#include "table.h"
#include "lvstring.h"
#include <cassert>

using namespace linguversa;
using namespace std;

Table::Table()
{
    m_pConnection = nullptr;
    m_hdbc = SQL_NULL_HDBC;
    m_hstmt = SQL_NULL_HSTMT;
}

Table::Table(Connection* pConnection)
{
    m_pConnection = nullptr;
    m_hdbc = SQL_NULL_HDBC;
    m_hstmt = SQL_NULL_HSTMT;
    SetDatabase(pConnection);
}

Table::~Table()
{
    try	// Do not terminate destructor by exception!
    {
        if (m_hstmt)
            Close();
    }
    catch (...) {}
}

void Table::SetDatabase(Connection* pConnection)
{
    if (pConnection != m_pConnection)
    {
        Close();
        m_pConnection = pConnection;
        m_hdbc = m_pConnection ? m_pConnection->GetSqlHDbc() : SQL_NULL_HDBC;
    }
}

void Table::SetDatabase(Connection& connection)
{
    if (&connection != m_pConnection)
    {
        Close();
        m_pConnection = &connection;
        m_hdbc = m_pConnection ? m_pConnection->GetSqlHDbc() : SQL_NULL_HDBC;
    }
}

SQLRETURN Table::LoadTableInfo(tstring tablename)
{
    SQLRETURN nRetCode = SQL_SUCCESS;    //Return code for your ODBC calls
    if (m_hdbc == NULL && m_pConnection != nullptr)
        m_hdbc = m_pConnection->GetSqlHDbc();
    assert(m_hdbc != SQL_NULL_HDBC);

    if (m_hdbc == NULL)
        return SQL_INVALID_HANDLE;

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

    // We ignore all parameters except m_hstmt and szTableName by setting them to NULL:
    //SQLTCHAR* szTableName = new TCHAR[tablename.length()+1];
    nRetCode = ::SQLColumns(m_hstmt, NULL, 0, NULL, 0,
        (SQLTCHAR*)tablename.c_str(), SQL_NTS, NULL, 0);

    if (!SQL_SUCCEEDED(nRetCode))
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        Close();
        return nRetCode;
    }

    SQLSMALLINT attrcount = 0; // number of meta data attributes for each column
    nRetCode = ::SQLNumResultCols(m_hstmt, &attrcount);
    if (!SQL_SUCCEEDED(nRetCode))
    {
        throw DbException(nRetCode, SQL_HANDLE_STMT, m_hstmt);
        Close();
        return nRetCode;
    }

    const size_t STR_LEN = 128 + 1;
    const size_t REM_LEN = 254 + 1;

    // Declare buffers for result set data  
    SQLTCHAR szSchema[STR_LEN];
    SQLTCHAR szCatalog[STR_LEN];
    SQLTCHAR szColumnName[STR_LEN];
    SQLTCHAR szTableName[STR_LEN];
    SQLTCHAR szTypeName[STR_LEN];
    SQLTCHAR szRemarks[REM_LEN];
    SQLTCHAR szColumnDefault[STR_LEN];
    SQLTCHAR szIsNullable[STR_LEN];

    SQLINTEGER ColumnSize;
    SQLINTEGER BufferLength;
    SQLINTEGER CharOctetLength;
    SQLINTEGER OrdinalPosition;

    SQLSMALLINT DataType;
    SQLSMALLINT DecimalDigits;
    SQLSMALLINT NumPrecRadix;
    SQLSMALLINT Nullable;
    SQLSMALLINT SQLDataType = 0;
    SQLSMALLINT DatetimeSubtypeCode;

    // Declare buffers for bytes available to return  
    SQLLEN cbCatalog = 0;
    SQLLEN cbSchema = 0;
    SQLLEN cbTableName = 0;
    SQLLEN cbColumnName = 0;
    SQLLEN cbDataType = 0;
    SQLLEN cbTypeName = 0;
    SQLLEN cbColumnSize = 0;
    SQLLEN cbBufferLength = 0;
    SQLLEN cbDecimalDigits = 0;
    SQLLEN cbNumPrecRadix = 0;
    SQLLEN cbNullable = 0;
    SQLLEN cbRemarks = 0;
    SQLLEN cbColumnDefault = 0;
    SQLLEN cbSQLDataType = 0;
    SQLLEN cbDatetimeSubtypeCode = 0;
    SQLLEN cbCharOctetLength = 0;
    SQLLEN cbOrdinalPosition = 0;
    SQLLEN cbIsNullable = 0;

    // Bind columns in result set to buffers  
    SQLBindCol(m_hstmt, 1, SQL_C_TCHAR, szCatalog, STR_LEN, &cbCatalog);
    SQLBindCol(m_hstmt, 2, SQL_C_TCHAR, szSchema, STR_LEN, &cbSchema);
    SQLBindCol(m_hstmt, 3, SQL_C_TCHAR, szTableName, STR_LEN, &cbTableName);
    SQLBindCol(m_hstmt, 4, SQL_C_TCHAR, szColumnName, STR_LEN, &cbColumnName);
    SQLBindCol(m_hstmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType);
    SQLBindCol(m_hstmt, 6, SQL_C_TCHAR, szTypeName, STR_LEN, &cbTypeName);
    SQLBindCol(m_hstmt, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize);
    SQLBindCol(m_hstmt, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength);
    SQLBindCol(m_hstmt, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits);
    SQLBindCol(m_hstmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix);
    SQLBindCol(m_hstmt, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable);
    SQLBindCol(m_hstmt, 12, SQL_C_TCHAR, szRemarks, REM_LEN, &cbRemarks);
    // the following column attributes exist only for ODBC 3.0 and higher
    if (attrcount >= 13)
    {
        SQLBindCol(m_hstmt, 13, SQL_C_TCHAR, szColumnDefault, STR_LEN, &cbColumnDefault);
        SQLBindCol(m_hstmt, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType);
        SQLBindCol(m_hstmt, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode);
        SQLBindCol(m_hstmt, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength);
        SQLBindCol(m_hstmt, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition);
        SQLBindCol(m_hstmt, 18, SQL_C_TCHAR, szIsNullable, STR_LEN, &cbIsNullable);
    }

    size_t col = 0;
    for (nRetCode = SQLFetch(m_hstmt); SQL_SUCCEEDED(nRetCode); nRetCode = SQLFetch(m_hstmt))
    {
        // fill vector _columnInfo with TableColumnInfo for each column
        if (col >= _columnInfo.size())
            _columnInfo.resize(col+10); // only increase vector size in reasonable steps
        // SQL_NO_TOTAL, SQL_NULL_DATA are < 0
        TableColumnInfo& tci = _columnInfo[col];
        szColumnName[cbColumnName > 0 ? cbColumnName : 0] = _T('\0');
        tci.ColumnName.assign((TCHAR*)szColumnName);
        tci.DataType = DataType;
        szTypeName[cbTypeName > 0 ? cbTypeName : 0] = _T('\0');
        tci.ColumnType.assign((TCHAR*)szTypeName);
        tci.SQLDataType = SQLDataType;
        tci.ColumnSize = ColumnSize;
        tci.BufferLength = BufferLength;
        tci.DecimalDigits = DecimalDigits;
        tci.NumPrecRadix = NumPrecRadix;
        tci.Nullable = Nullable;
        col++;
    }

    // probably we increased vector size a bit too much, decrease to the real number of columns
    _columnInfo.resize(col);

    if (nRetCode == SQL_NO_DATA)
        nRetCode = SQL_SUCCESS;

    return nRetCode;

}

short int Table::GetColumnIndex(tstring colname) const
{
    for (size_t col = 0; col < _columnInfo.size(); col++)
    {
        if (lower(colname) == lower(_columnInfo[col].ColumnName))
            return (short int) col;
    }
    return -1;
}

const TableColumnInfo& Table::GetColumnInfo(size_t col) const
{
    return _columnInfo[col];
}

SQLRETURN Table::Close()
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

    m_hstmt = SQL_NULL_HSTMT;

    return nRetCode;
}

