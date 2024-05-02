#include "target.h"
#include "query.h"
#include "lvstring.h"
#include <sstream>
#include <cassert>

#ifdef _WIN32
// various Window platforms:
// - Multi_Byte character set or UNICODE
// - x86 or x64 architecture
// - MSVC or MINGW compiler
#ifdef UNICODE
#define tostringstream wostringstream
#else
#define tostringstream ostringstream
#endif
#else
// Non-Windows PLatforms
#define tostringstream ostringstream
#define SI_NO_CONVERSION
#include <unistd.h>
#endif


using namespace std;
using namespace linguversa;

Target::Target()
{
	selector = sel_stdout;
}

Target::~Target()
{
	Close();
}

bool Target::OpenStdOutput()
{
    Close();
    selector = sel_stdout;
    return true;
}

bool Target::OpenConnection(std::tstring connection)
{
	Close();
	bool ret = m_connnection.Open(connection);
	selector = ret ? sel_odbc : sel_stdout;
	return ret;
}

bool Target::OpenFile(std::tstring filepath)
{
	Close();
	try {
		m_outstream.open(filepath);
	}
	catch (...) {
		selector = sel_stdout;
		return false;
	}

	if (m_outstream.is_open())
	{
		selector = sel_fileout;
		return true;
	}
	else
	{
		selector = sel_stdout;
		return false;
	}
}

bool Target::IsOpen()
{
	return m_outstream.is_open() || m_connnection.IsOpen();
}

void Target::Close()
{
	m_outstream.close();
	m_connnection.Close();
	selector = sel_stdout;
}

bool Target::Apply(std::tstring str)
{
	bool ret = false;
	if (selector == sel_odbc && m_connnection.IsOpen())
	{
		Query query(&m_connnection);
		ret = query.ExecDirect(str);
	}
	else if (selector == sel_fileout || selector == sel_stdout)
	{
		// output stream os, may either be the just opened file or otherwise stdout
		tostream& os = (selector == sel_fileout && m_outstream.is_open() ? m_outstream : tcout);
		os << str << endl;
		ret = true;
	}
	return ret;
}

void Target::Create(const Query& query, tstring tablename)
{
    if (tablename.length() == 0)
        return;
    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

    tostringstream os;
    os << string_format(_T("create table %s(\n"), tablename.c_str());
    // ***********************************************************************
    // Retrieve meta information on the columns of the result set
    // this is NOT mandatory for the subsequent retrieval of the column values
    // ***********************************************************************
    for (short col = 0; col < colcount; col++)
    {
        FieldInfo fieldinfo;
        tstring sqltypename;
        query.GetODBCFieldInfo(col, fieldinfo);
        switch (fieldinfo.m_nSQLType)
        {
        case SQL_TINYINT:
            sqltypename = _T("BYTE");
            break;
        case SQL_SMALLINT:
            sqltypename = _T("SMALLINT");
            break;
        case SQL_INTEGER:
            sqltypename = _T("INTEGER");
            break;
        case SQL_REAL:
            sqltypename = _T("REAL");
            break;
        case SQL_DOUBLE:
            sqltypename = _T("DOUBLE");
            break;
        case SQL_DATETIME:
            sqltypename = _T("DATETIME");
            break;
        case SQL_TYPE_DATE:
            sqltypename = _T("DATE");
            break;
        case SQL_TIME:
            sqltypename = string_format(_T("TIME(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_TYPE_TIME:
            sqltypename = _T("TIME");
            break;
        case SQL_TIMESTAMP:
            sqltypename = _T("DATETIME");
            break;
        case SQL_TYPE_TIMESTAMP:
            sqltypename = _T("DATETIME");
            break;
        case SQL_CHAR:
            sqltypename = string_format(_T("CHAR(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_VARCHAR:
            sqltypename = string_format(_T("VARCHAR(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_WCHAR:
            sqltypename = string_format(_T("NCHAR(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_WVARCHAR:
            sqltypename = string_format(_T("NVARCHAR(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_BINARY:
            sqltypename = string_format(_T("BINARY(%01d)"), fieldinfo.m_nPrecision);
            break;
        case SQL_DECIMAL:
            sqltypename = string_format(_T("DECIMAL(%01d,%01d)"), fieldinfo.m_nPrecision, fieldinfo.m_nScale);
            break;
        case SQL_NUMERIC:
            sqltypename = string_format(_T("NUMERIC(%01d,%01d)"), fieldinfo.m_nPrecision, fieldinfo.m_nScale);
            break;
        case SQL_BIGINT:
        case SQL_BIGINT + SQL_UNSIGNED_OFFSET:
            sqltypename = _T("BIGINT");
            break;
        case SQL_GUID:
            sqltypename = _T("UNIQUE_IDENTIFIER");
            break;
        default:
            sqltypename = _T("UNKNOWN");
            break;
        }

        tstring sNullable;
        switch (fieldinfo.m_nNullability)
        {
        case SQL_NULLABLE:
            sNullable = _T("NULL");
            break;
        case SQL_NO_NULLS:
            sNullable = _T("NOT NULL");
            break;
        }

        os << ::string_format(_T("\t%s %s %s%s"),
            fieldinfo.m_strName.c_str(), sqltypename.c_str(), sNullable.c_str(),
            (col < colcount - 1) ? _T(",\n") : _T("\n);"));
    }

    Apply(os.str());
}

void Target::InsertCurrentRow(Query& query, tstring tablename)
{
    if (tablename.length() == 0)
        return;

    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

    // TODO: avoid invalid characters in tablename
    tostringstream os;
    os << ::string_format(_T("insert into %01s( "), tablename.c_str());
    // ***********************************************************************
    // Retrieve meta information on the columns of the result set.
    // ***********************************************************************
    for (short col = 0; col < colcount; col++)
    {
        FieldInfo fieldinfo;
        query.GetODBCFieldInfo(col, fieldinfo);
        os << fieldinfo.m_strName;
        if (col < colcount - 1)
            os << _T(", ");
    }
    os << _T(")") << endl;

    os << _T("values( ");
    for (short col = 0; col < colcount; col++)
    {
        DBItem dbitem;
        lvstring str;
        FieldInfo fi;
        query.GetFieldValue(col, dbitem);
        switch (dbitem.m_nVarType)
        {
        case DBItem::lwvt_null:
            os << _T("NULL");
            break;
        case DBItem::lwvt_astring:
        case DBItem::lwvt_wstring:
        case DBItem::lwvt_string:
            str = DBItem::ConvertToString(dbitem);
            // change ' to '':
            str.Replace(_T("'"), _T("''"));
            os << _T("'") << str << _T("'");
            break;
        case DBItem::lwvt_guid:
            os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
            break;
        case DBItem::lwvt_date:
            os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
            break;
        case DBItem::lwvt_single:
        case DBItem::lwvt_double:
            // don't omit decimal places
            query.GetODBCFieldInfo(col, fi);
            if (fi.m_nSQLType == SQL_NUMERIC || fi.m_nSQLType == SQL_DECIMAL)
                os << DBItem::ConvertToString(dbitem, fi.GetDefaultFormat());
            else
                os << DBItem::ConvertToString(dbitem);
            break;
        default:
            os << DBItem::ConvertToString(dbitem);
        }
        if (col < colcount - 1)
            os << _T(", ");
    }
    os << _T(");");
    Apply(os.str());
}

void Target::InsertAll(Query& query, const tstring tablename)
{
    if (tablename.length() == 0)
        return;

    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

    // TODO: avoid invalid characters in tablename
    tostringstream os;
    // ***********************************************************************
    // Now we retrieve data by iterating over the rows of the result set.
    // If Result set has 0 rows it will skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    bool bFirstRow = true;
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        if (bFirstRow)
        {
            os << ::string_format(_T("insert into %01s( "), tablename.c_str());
            // ***********************************************************************
            // Retrieve meta information on the columns of the result set.
            // ***********************************************************************
            for (short col = 0; col < colcount; col++)
            {
                FieldInfo fieldinfo;
                query.GetODBCFieldInfo(col, fieldinfo);
                os << fieldinfo.m_strName;
                if (col < colcount - 1)
                    os << _T(", ");
            }
            os << _T(")") << endl;
            os << _T("select ");
            bFirstRow = false;
        }
        else
        {
            os << endl << _T("union all") << endl << _T("select ");
        }

        for (short col = 0; col < colcount; col++)
        {
            DBItem dbitem;
            lvstring str;
            FieldInfo fi;
            query.GetFieldValue(col, dbitem);
            switch (dbitem.m_nVarType)
            {
            case DBItem::lwvt_null:
                os << _T("NULL");
                break;
            case DBItem::lwvt_astring:
            case DBItem::lwvt_wstring:
            case DBItem::lwvt_string:
                str = DBItem::ConvertToString(dbitem);
                // change ' to '':
                str.Replace(_T("'"), _T("''"));
                os << _T("'") << str << _T("'");
                break;
            case DBItem::lwvt_guid:
                os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
                break;
            case DBItem::lwvt_date:
                os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
                break;
            case DBItem::lwvt_single:
            case DBItem::lwvt_double:
                // don't omit decimal places
                query.GetODBCFieldInfo(col, fi);
                if (fi.m_nSQLType == SQL_NUMERIC || fi.m_nSQLType == SQL_DECIMAL)
                    os << DBItem::ConvertToString(dbitem, fi.GetDefaultFormat());
                else
                    os << DBItem::ConvertToString(dbitem);
                break;
            default:
                os << DBItem::ConvertToString(dbitem);
            }
            if (col < colcount - 1)
                os << _T(", ");
        }
    }

    os << _T(";");
    Apply(os.str());
}

void Target::OutputAsCSV(Query& query, const tstring& fieldseparator)
{
    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

    tostringstream os;
    // ***********************************************************************
    // Retrieve meta information on the columns of the result set
    // this is NOT mandatory for the subsequent retrieval of the column values
    // ***********************************************************************
    for (short col = 0; col < colcount; col++)
    {
        FieldInfo fieldinfo;
        query.GetODBCFieldInfo(col, fieldinfo);
        os << fieldinfo.m_strName;
        if (col == colcount - 1)
            os << endl;
        else
            os << fieldseparator;
    }

    // ***********************************************************************
    // Now we retrieve data by iterating over the rows of the result set.
    // If Result set has 0 rows it will skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        for (short col = 0; col < colcount; col++)
        {
            os << query.FormatFieldValue(col);
            if (col == colcount - 1)
                os << endl;
            else
                os << fieldseparator;
        }
    }

    Apply(os.str());
}

