// qx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "qx.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <exception>
#include <cassert>
#include "external/CLI11.hpp"
#include "external/SimpleIni.h"
#include "../query/query.h"
#include "../query/odbcenvironment.h"
#include "../query/lvstring.h"
#include "../query/target.h"
#ifndef UNICODE
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif
#include "external/csv.hpp"
#ifdef _MSC_VER
#pragma warning( pop )
#endif
#endif

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
#include <filesystem>
#endif

using namespace std;
using namespace linguversa;
using namespace CLI;

//forward declarations
void ListDrivers();
void ListDataSources(tstring drivername = _T(""));
void OutputResultSet(tostream& os, Query& query, const tstring& fieldseparator);
void BuildConnectionstring( tstring& sourcepath, tstring& connectionstring, tstring& stmt);
void ConfigToSchema(tstring filepath, tstring configname);
string validateSqliteFilename( string& path);


int main(int argc, char** argv) 
{
    CLI::App app{ "qx - ODBC Query eXecuter" };

    tstring connectionstring;
    tstring sourcepath;
    tstring dirpath;
    tstring dbasedir;
    tstring sqlite3;
    tstring csvfile;
    tstring config;
    tstring fieldseparator = _T("\t");
    tstring rowformat;
    tstring input;
    tstring outputfile;
    tstring targetspec;
    tstring create;
    tstring insert;
    tstring createinsert;
    bool verbose = false;
    bool listdrivers = false;
    bool listdsn = false;
    vector<tstring> sqlcmd;

    app.add_flag("-v,--verbose", verbose, "verbose output");
    app.add_flag("--listdrivers", listdrivers, "list installed ODBC drivers");
    app.add_flag("--listdsn", listdsn, "list ODBC data sources");
    app.add_option("-s,--source", connectionstring, "ODBC connection string")
        ->envname("QXCONNECTION");
    app.add_option("--sourcepath", sourcepath, "path of a textfile or database directory")
        ->excludes("--source")
        ->check(CLI::ExistingPath | CLI::Validator(validateSqliteFilename, "*.sqlite or *.db3"));
#ifndef UNICODE
    app.add_option("--csvfile", csvfile, "path of a character separated file")
        ->check(CLI::ExistingPath);
#endif
    app.add_option("--config", config, "template in qx.ini for file description in schema.ini-format");
    app.add_option("--sqlite3", sqlite3, "path of a sqlite3 database file")
        ->excludes("--source")->excludes("--sourcepath");
    app.add_option("--dir", dirpath, "path of a database directory containing textfiles")
        ->excludes("--sqlite3")->excludes("--source")->excludes("--sourcepath")
        ->check(CLI::ExistingDirectory);
    app.add_option("--dbase", dbasedir, "path of a database directory containing dbase files")
        ->excludes("--sqlite3")->excludes("--source")->excludes("--sourcepath")->excludes("--dir")
        ->check(CLI::ExistingDirectory);
    app.add_option("-f,--format", rowformat, "row format");
    app.add_option("--fieldseparator", fieldseparator, "fieldseparator (Default is TAB)")->excludes("--format");
    app.add_option("--create", create, "generate create statement for specified tablename")->excludes("--format")->excludes("--fieldseparator");
    app.add_option("--insert", insert, "generate insert statements for specified tablename")->excludes("--format")->excludes("--fieldseparator");
    app.add_option("--createinsert", createinsert, "generate create and insert statements for specified tablename")
        ->excludes("--format")->excludes("--fieldseparator")->excludes("--create")->excludes("--insert");
    app.add_option("--input", input, "filepath of input file containing SQL statements")
        ->check(CLI::ExistingFile | CLI::Validator([](string& s) { return s == "stdin" ? "" : "stdin"; }, "stdin"));;
    app.add_option("--outputfile", outputfile, "filepath of output file");
    app.add_option("--target", targetspec, "target for output")->excludes("--outputfile");
    app.add_option("sqlcmd", sqlcmd, "SQL-statement(s) (each enclosed in \"\" and space-separated)");

    try {
        argv = app.ensure_utf8(argv);
        app.parse(argc, argv);
    } catch(const CLI::ParseError& e) {
        return app.exit(e);
    }

    if (createinsert.length() > 0)
    {
        create = createinsert;
        insert = createinsert;
    }

    tofstream ofs;
    if (outputfile.length() > 0)
        ofs.open(outputfile);

    // output stream os, may either be the just opened file or otherwise stdout
    tostream& os = ofs.is_open() ? ofs : tcout;

    Target target;
    if (targetspec.length())
    {
        bool ret = true;
        if (lower(targetspec.substr(0, 5)) == _T("file:"))
        {
            ret = target.OpenFile(targetspec.substr(5));
        }
        else if (lower(targetspec.substr(0, 5)) == _T("odbc:") || lower(targetspec.substr(0, 5)) == _T("odbc;"))
        {
            ret = target.OpenConnection(targetspec.substr(5));
        }
        else if (lower(targetspec) == _T("stdout"))
        {
            ret = target.OpenStdOutput();
        }
        
        if (ret == false)
            return -1;
    }

    SQLRETURN nRetCode = SQL_SUCCESS;

    try
    {
        // first get information independent of specific connection
        if (listdrivers)
        {
            ::ListDrivers();
        }

        if (listdsn) 
        {
            ::ListDataSources();
        }
    }
    catch(DbException& ex) 
    {
        nRetCode = ex.getSqlCode();
        tcerr << _T("Cannot read list of installed drivers:") << endl << ex.what() << endl;
    }

    if (dirpath.length() > 0)
        sourcepath = dirpath;

    if (sourcepath.length() > 0)
    {
        tstring stmt;
        // build connectionstring fromn sourcepath parameter
        BuildConnectionstring( sourcepath, connectionstring, stmt);
        if (sqlcmd.size() == 0 && stmt.length() > 0)
            sqlcmd.push_back(stmt);
    }
    else if (dbasedir.length() > 0)
    {
        connectionstring = ::string_format(_T("Driver={Microsoft dBase Driver (*.dbf)};DriverID=277;Dbq=%s;"), dbasedir.c_str());
    }
    else if (sqlite3.length() > 0)
    {
#ifdef _WIN32
        connectionstring = ::string_format(_T("Driver={SQLite3 ODBC Driver};Database=%s;"), sqlite3.c_str());
#else
        connectionstring = ::string_format(_T("Driver=SQLite3;Database=%s;"), sqlite3.c_str());
#endif
    }

    // connectionstring can be specified (in order of priortity)
    // 1. as option --source on the command line
    
    // 2. in environment variable QXCONNECTION

    // 3. as value ConnectionString in qx.ini
    if (connectionstring.length() == 0)
    {
        CSimpleIni ini;
        SI_Error rc = ini.LoadFile("qx.ini");
        if (rc == SI_OK)
        {
            const TCHAR* cs = ini.GetValue(_T("Database"), _T("ConnectionString"), _T(""));
            if (cs)
                connectionstring.assign(cs);
        }
    }

    if (verbose) 
    {
        os << "Connection string: " << connectionstring << endl;
        if (rowformat.length() > 0)
            os << "Format: " << rowformat << endl;

        for(unsigned int i = 0; i < sqlcmd.size(); i++)
        {
            os << sqlcmd[i] << endl;
        }

        os << endl;
    }

    if (csvfile.length() == 0 && connectionstring.length() == 0 && sqlcmd.size() == 0)
    {
        if (rowformat.length() > 0)
            os << "Format: " << rowformat << endl;

        target.Close();
        return 0;
    }

    // Microsoft Text Driver only: 
    // copy schema section from qx.ini to schema.ini
    if (config.length() > 0 && connectionstring.substr(0,22) == _T("Driver={Microsoft Text"))
    {
        ConfigToSchema(sourcepath, config);
    }

    //rowformat = _T("[lfnbr:%5d]\\n");
    lvstring s = rowformat;
    s.Replace(_T("\\n"), _T("\n"));
    s.Replace(_T("\\t"), _T("\t"));
    rowformat = s;

    s = fieldseparator;
    s.Replace(_T("\\n"), _T("\n"));
    s.Replace(_T("\\t"), _T("\t"));
    fieldseparator = s;

#ifndef UNICODE
    if (csvfile.length() > 0)
    {
        csv::CSVReader reader(csvfile);
        for (csv::CSVRow& row : reader) // Input iterator
        {
            // TODO:
            if (rowformat.length() > 0)
            {
                // ***********************************************************************
                // Iterate over the rows of the current result set. 
                // If Result set has 0 rows it will skip the loop because nRetCode is set 
                // to SQL_NO_DATA immediately
                // ***********************************************************************
                //for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
                //{
                //    // apply user-defined rowformat to each row.
                //    target.Apply(query.FormatCurrentRow(rowformat));
                //}
            }
            else if (insert.length() > 0)
            {
                // Iterate over all rows of the curnnt result set and
                // create one insert statement for all rows.
                //::GenerateInsert(sstream, query, insert);
                // create one insert statement for all rows.
                //target.InsertAll(query, insert);
            }
            else
            {
                for (csv::CSVField& field : row)
                {
                    // By default, get<>() produces a std::string.
                    // A more efficient get<string_view>() is also available, where the resulting
                    // string_view is valid as long as the parent CSVRow is alive
                    std::cout << field.get<>() << fieldseparator;
                }
            }
            std::cout << endl;
        }

        if (connectionstring.length() == 0)
            return 0;
    }
#endif

    // now open a real connection as specified by connectionstring
    bool b = false;
    Connection con;
    Query query;

    try
    {
        b = con.Open( connectionstring);
        if (b)
            query.SetDatabase(con);
    } 
    catch(DbException& ex)
    {
        nRetCode = ex.getSqlCode();
        tcerr << _T("Cannot open connection:") << endl;
        cerr << ex.what() << endl;
        if (ofs.is_open())
            ofs.close();
        target.Close();
        return nRetCode;
    } 
    catch(...) 
    {
        tcerr << _T("Cannot open connection!") << endl;
        if (ofs.is_open())
            ofs.close();
        target.Close();
        return -1;
    }

    if (verbose)
    {
        if(b) 
            os << "Connection opened." << endl;
        else
            os << "Cannot open connection." << endl;
    }

    if (!b)
    {
        if (ofs.is_open())
            ofs.close();
        target.Close();
        return -1;
    }

    // ready to execute real sql statements (from command line parameters)
    for (size_t n = 0; n < sqlcmd.size(); n++)
    {
        tstring sql = sqlcmd[n];
        if (verbose)
            os << sql << endl;

        try
        {
            nRetCode = query.ExecDirect(sql);

            if (SQL_SUCCEEDED(nRetCode) && create.length() > 0)
            {
                target.Create(query, create);
            }

            // Even a single statement (or.batch of statements) can have more than one result set.
            // Iterate over all result sets:
            while (SQL_SUCCEEDED(nRetCode))
            {
                if (rowformat.length() > 0)
                {
                    // ***********************************************************************
                    // Iterate over the rows of the current result set. 
                    // If Result set has 0 rows it will skip the loop because nRetCode is set 
                    // to SQL_NO_DATA immediately
                    // ***********************************************************************
                    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
                    {
                        // apply user-defined rowformat to each row.
                        target.Apply(query.FormatCurrentRow(rowformat));
                    }
                }
                else if (insert.length() > 0)
                {
                    // Iterate over all rows of the curnnt result set and
                    // create one insert statement for all rows.
                    //::GenerateInsert(sstream, query, insert);
                    // create one insert statement for all rows.
                    target.InsertAll(query, insert);
                }
                else
                {
                    // Output the complete current result set in standard format.
                    target.OutputAsCSV(query, fieldseparator);
                }

                // there may be more result sets ...
                nRetCode = query.SQLMoreResults();
            }
        }
        catch (DbException& ex)
        {
            nRetCode = ex.getSqlCode();
            tcerr << _T("Execute error:") << endl;
            cerr << ex.what() << endl;
            if (ofs.is_open())
                ofs.close();
            target.Close();
            return nRetCode;
        }
    }

    // interactive mode (console) or input from file
    if (input.length() > 0)
    {
        tifstream ifs;
        if (input != _T("stdin")) // we read from normal file
            ifs.open(input);

        tistream& is = ifs.is_open() ? ifs : tcin;
        tstring buf;
        tstringstream sql;
        bool quoted = false;

        // we use prompt only if we are in normal tty input and output
        bool bCommandPrompt = (input == _T("stdin")) && ISATTY(FILENO(stdin)) && ISATTY(FILENO(stdout));
        if (bCommandPrompt)
            tcout << _T("qx> ");
        while (getline(is, buf))
        {
            if (!quoted && (buf == _T("quit") || buf == _T("exit")))
                break;

            bool comment = false;
            if (sql.str().length() > 0)
                sql << _T('\n');

            for (size_t i = 0; i < buf.length(); i++)
            {
                switch (buf[i])
                {
                case _T(';'):
                    sql << buf[i];
                    if (!quoted && !comment)
                    {
                        // do something with sql ...
                        try
                        {
                            nRetCode = query.ExecDirect(sql.str());

                            // Even a single statement (or.batch of statements) can have more than one result set.
                            // Iterate over all result sets:
                            while (SQL_SUCCEEDED(nRetCode))
                            {
                                // Output the complete current result set in standard format.
                                ::OutputResultSet(os, query, fieldseparator);

                                // there may be more result sets ...
                                nRetCode = query.SQLMoreResults();
                            }
                        }
                        catch (DbException& ex)
                        {
                            nRetCode = ex.getSqlCode();
                            tcerr << _T("Execute error:") << endl;
                            cerr << ex.what() << endl;
                        }

                        // and afterwards reset the sql to empty string
                        sql.str(std::tstring());
                        // ignore following spaces
                        while (i + 1 < buf.length() && (buf[i + 1] == _T(' ') || buf[i + 1] == _T('\t')))
                            i++;
                    }
                    break;
                case _T('\''):
                    sql << buf[i];
                    quoted = !quoted;
                    break;
                case _T('-'):
                    if (!quoted && i + 1 < buf.length() && buf[i + 1] == _T('-'))
                        comment = true;
                    else
                        sql << buf[i];
                    break;
                default:
                    sql << buf[i];
                }

                if (comment)
                    break;
            }

            if (bCommandPrompt)
            {
                // if sql is empty do a normal prompt, otherwise a different prompt
                tcout << (sql.str().length() > 0 ? _T(">>> ") : _T("\nqx> "));
            }
        }

        if (ifs.is_open())
            ifs.close();
    }

    if (ofs.is_open())
        ofs.close(); // close the output file stream

    target.Close();
    query.Close();
    con.Close();
    
    if (nRetCode == SQL_NO_DATA) // end of data successfully reached
        nRetCode = SQL_SUCCESS;

    return nRetCode;
}

/*
void FormatResultSet(tostream& os, Query& query, const tstring& rowformat)
{
    // ***********************************************************************
    // Iterate over the rows of the result set. if Result set has 0 rows it will 
    // skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        os << query.FormatCurrentRow(rowformat);
    }
}
*/

void OutputResultSet(tostream& os, Query& query, const tstring& fieldseparator)
{
    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

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
            //DBItem dbitem;
            //query.GetFieldValue(col, dbitem);
            //os << DBItem::ConvertToString(dbitem);
            os << query.FormatFieldValue(col);
            if (col == colcount - 1)
                os << endl;
            else
                os << fieldseparator;
        }
    }
}

void ListDrivers()
{
    tcout << _T("ODBCDrivers:") << endl;

    ODBCEnvironment environment;
    tstring driver;
    vector<tstring> attributes;
    for (SQLRETURN nRetCode = environment.FetchDriverInfo(driver, attributes, SQL_FETCH_FIRST);
        SQL_SUCCEEDED(nRetCode);
        nRetCode = environment.FetchDriverInfo(driver, attributes))
    {
        tcout << driver << endl;
        for (unsigned short i = 0; i < attributes.size(); i++)
            tcout << _T("\t") << attributes[i] << endl;
    }

    tcout << endl;
}

void ListDataSources(tstring drivername)
{
    tcout << "ODBC data sources:" << endl;

    ODBCEnvironment environment;
    tstring dsn;
    tstring drivername;
    for (SQLRETURN nRetCode = environment.FetchDataSourceInfo(dsn, drivername, SQL_FETCH_FIRST); SQL_SUCCEEDED(nRetCode);
        nRetCode = environment.FetchDataSourceInfo(dsn, drivername))
    {
        tcout << dsn << _T("|") << drivername << endl;
    }

    tcout << endl;
}

/*
void GenerateCreate(tostream& os, Query& query, const tstring& tablename)
{
    if (tablename.length() == 0)
        return;
    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

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
        //case SQL_DATE:
        //    sqltypename = _T("DATE");
        //    break;
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
            (col < colcount -1) ? _T(",\n") : _T("\n);\n"));
    }
}

void GenerateInsert(tostream& os, Query& query, const tstring& tablename)
{
    if (tablename.length() == 0)
        return;

    short colcount = query.GetODBCFieldCount();
    if (colcount <= 0)
        return;

    tstring insertstmt = ::string_format(_T("insert into %01s( "), tablename.c_str());
    tstring strval;
    // ***********************************************************************
    // Retrieve meta information on the columns of the result set.
    // ***********************************************************************
    for (short col = 0; col < colcount; col++)
    {
        FieldInfo fieldinfo;
        query.GetODBCFieldInfo(col, fieldinfo);
        insertstmt += fieldinfo.m_strName;
        if (col < colcount - 1)
            insertstmt += _T(", ");
    }
    insertstmt += _T(")");

    // ***********************************************************************
    // Now we retrieve data by iterating over the rows of the result set.
    // If Result set has 0 rows it will skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        os << insertstmt << endl << _T("values( ");
        for (short col = 0; col < colcount; col++)
        {
            DBItem dbitem;
            query.GetFieldValue(col, dbitem);
            switch (dbitem.m_nVarType)
            {
            case DBItem::lwvt_null:
                os << _T("NULL");
                break;
            case DBItem::lwvt_astring:
            case DBItem::lwvt_wstring:
            case DBItem::lwvt_string:
                os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
                break;
            case DBItem::lwvt_guid:
                os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
                break;
            case DBItem::lwvt_date:
                os << _T("'") << DBItem::ConvertToString(dbitem) << _T("'");
                break;
            default:
                os << DBItem::ConvertToString(dbitem);
            }
            //os << (dbitem.m_nVarType != DBItem::lwvt_null ? DBItem::ConvertToString(dbitem) : _T("NULL"));
            if (col < colcount - 1)
                os << _T(", ");
        }
        os << _T(")") << endl;
    }
}
*/

void BuildConnectionstring( tstring& sourcepath, tstring& connectionstring, tstring& stmt)
{
    size_t len = sourcepath.length();
    bool isSQLite = (len >= 4 && sourcepath.substr(len - 4) == _T(".db3")) 
        || (len >= 8 && sourcepath.substr(len - 8) == _T(".sqlite3"));
    stmt.clear();
    struct ::_stat64 spattr;
    if (::_tstat64(sourcepath.c_str(), &spattr) != 0)
    {
        // error, except when of type sqlite
    }
    else if (spattr.st_mode & S_IFDIR) // directory
    {
#ifdef _WIN32
        // build connectionstring from sourcepath path, assuming it is text based db
        TCHAR* fp = ::_tfullpath(nullptr, sourcepath.c_str(), _MAX_PATH);
        if (fp)
        {
            connectionstring = ::string_format(
                _T("Driver={Microsoft Text Driver (*.txt; *.csv)};Dbq=%s;Extensions=asc,csv,tab,txt;"), fp);
            free(fp);
        }
#else
        // linux:
        TCHAR* fp = ::realpath(sourcepath.c_str(), nullptr);
        free(fp);
#endif
    }
    else if (isSQLite || spattr.st_mode & S_IFREG) // regular file
    {
#ifdef _WIN32
        // build connectionstring from sourcepath path, assuming it is text based db
        TCHAR* fp = ::_tfullpath(nullptr, sourcepath.c_str(), _MAX_PATH);
        if (fp)
        {
            sourcepath.assign(fp);
            free(fp);
        }

        if (isSQLite) // sqlite3 db dile
        {
            connectionstring = ::string_format(_T("Driver={SQLite3 ODBC Driver};Database=%s;"), sourcepath.c_str());
            return;
        }

        len = sourcepath.length();
        TCHAR drv[_MAX_DRIVE];
        TCHAR dir[_MAX_DIR];
        TCHAR fname[_MAX_FNAME];
        TCHAR fext[_MAX_EXT];
        errno_t errnbr = _tsplitpath_s(sourcepath.c_str(), drv, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, fext, _MAX_EXT);
        assert(errnbr == 0);
        if (errnbr != 0)
            return;
        tstring filename = string_format(_T("%s%s"), fname, fext);
        if (len >= 5 && sourcepath.substr(len - 4) == _T(".dbf"))
        {
            connectionstring = string_format(_T("Driver={Microsoft dBase Driver (*.dbf)};DriverID=277;Dbq=%s%s;"), drv, dir);
            stmt = string_format(_T("select * from %s;"), fname);
            return;
        }
        else if (len > 4 && (sourcepath.substr(len - 4) == _T(".csv") || sourcepath.substr(len - 4) == _T(".tsv")
            || sourcepath.substr(len - 4) == _T(".tab") || sourcepath.substr(len - 4) == _T(".txt")))
        {
            connectionstring = string_format(
                _T("Driver={Microsoft Text Driver (*.txt; *.csv)};Dbq=%s%s;Extensions=asc,csv,tab,txt;"), drv, dir);
            stmt = string_format(_T("select * from [%s];"), filename.c_str());
            return;
        }
#else
        // linux:
        TCHAR* fp = ::realpath(sourcepath.c_str(), nullptr);
        sourcepath.assign(fp);
        free(fp);
        len = sourcepath.length();
        
        if (isSQLite) // sqlite3 db dile
        {
            connectionstring = ::string_format(_T("Driver=SQLite3;Database=%s;"), sourcepath.c_str());
            return;
        }
#endif

        if (len >= 4 && sourcepath.substr(len - 4) == _T(".mdb"))
        {
            connectionstring = ::string_format(
                _T("Driver={Microsoft Access Driver (*.mdb)};Dbq=%s;Uid=Admin;Pwd=;"), sourcepath.c_str());
        }
    }
}

void ConfigToSchema(tstring filepath, tstring configname)
{
    size_t len = filepath.length();
    if (configname.length() > 0 && len > 4 &&
        (filepath.substr(len - 4) == _T(".csv") || filepath.substr(len - 4) == _T(".tsv")
            || filepath.substr(len - 4) == _T(".tab") || filepath.substr(len - 4) == _T(".txt")))
    {
#ifdef WIN32
        // Windows
        TCHAR drv[_MAX_DRIVE];
        TCHAR dir[_MAX_DIR];
        TCHAR fname[_MAX_FNAME];
        TCHAR fext[_MAX_EXT];
        errno_t errnbr = _tsplitpath_s(filepath.c_str(), drv, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, fext, _MAX_EXT);
        assert(errnbr == 0);
        if (errnbr != 0)
            return;
        tstring filename = string_format(_T("%s%s"), fname, fext);
        tstring schemapath = string_format(_T("%s%s\\schema.ini"), drv, dir);
#elif ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
        // Un*x, C++ 17 or higher
        std::filesystem::path fpath(filepath);
        tstring schemapath = string_format(_T("%s\\schema.ini"), fpath.parent_path().string().c_str());
        tstring filename = fpath.filename().string();
#else 
        // Un*x, before C++ 11
        std::size_t botDirPos = filepath.find_last_of("/");
        tstring schemapath = string_format(_T("%s\\schema.ini"), filepath.substr(0, botDirPos).c_str());
        tstring filename = filepath.substr(botDirPos, filepath.length());
#endif

        CSimpleIni schema_ini;
        tstring sectionname = string_format(_T("QueryConfig\\%s\\schema"), configname.c_str());
        SI_Error rc = schema_ini.LoadFile(schemapath.c_str());
        // Only insert new schema, do not replace already existing schema!
        if (schema_ini.GetSectionSize(filename.c_str()) <= 0)
        {
            CSimpleIni qx_ini;
            CSimpleIni::TNamesDepend keys;
            if (qx_ini.LoadFile(_T("qx.ini")) != SI_OK || qx_ini.GetAllKeys(sectionname.c_str(), keys) == false)
                return;

            keys.sort(CSimpleIni::Entry::LoadOrder());
            for (std::list<CSimpleIni::Entry>::iterator it = keys.begin(); it != keys.end(); ++it)
            {
                const TCHAR* pv = qx_ini.GetValue(sectionname.c_str(), it->pItem, _T(""));
                rc = schema_ini.SetValue(filename.c_str(), it->pItem, pv ? pv : _T(""), (const TCHAR*)0, true);
            }

            if (schema_ini.GetSectionSize(filename.c_str()) > 0 && schema_ini.SaveFile(schemapath.c_str()) != SI_OK)
                return;
        }
    }
}

string validateSqliteFilename( string& path)
{
    size_t len = path.length();
    if ((len >= 8 && path.substr(len-8) == ".sqlite3") || (len >= 4 && path.substr(len-4) == ".db3")) 
        return "";
    else
        return "If file does not exist, extension must be .sqlite3 or .db3.";
}
