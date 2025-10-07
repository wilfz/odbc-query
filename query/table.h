#include "connection.h"
#include "dbitem.h"
#include "fieldinfo.h"
#include "resultinfo.h"

#define USE_ROWDATA
#ifdef USE_ROWDATA
#include "datarow.h"
#endif

namespace linguversa
{
    typedef struct {
        std::tstring ColumnName;
        SQLSMALLINT DataType;
        std::tstring ColumnType;
        SQLSMALLINT SQLDataType;
        SQLINTEGER ColumnSize;
        SQLINTEGER BufferLength;
        SQLSMALLINT DecimalDigits;
        SQLSMALLINT NumPrecRadix;
        SQLSMALLINT Nullable;
    } TableColumnInfo;

    class Table
    {
    public:
        Table();
        Table(Connection* pConnection);
        ~Table();

        // Set the database handle to a Connection object which encapsulates the ODBC connection.
        void SetDatabase(Connection* pConnection);
        void SetDatabase(Connection& connection);

        SQLRETURN LoadTableInfo(std::tstring tablename);
        size_t GetColumnCount() const { return _columnInfo.size(); };
        short int GetColumnIndex(std::tstring colname) const;
        const TableColumnInfo& GetColumnInfo(size_t col) const;

        // Finish the statement and release all its resources.
        SQLRETURN Close();

    protected:
        Connection* m_pConnection;
        HDBC m_hdbc;
        HSTMT m_hstmt;
        std::vector<TableColumnInfo> _columnInfo;
    };
}
