// qx.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <stdio.h>
#include <exception>
#include "CLI11.hpp"
#include "../query/tstring.h"
#include "SimpleIni.h"
#include "../query/query.h"
#include "../query/odbcenvironment.h"
#include "../query/lvstring.h"

using namespace std;
using namespace linguversa;
using namespace CLI;

//forward declarations
void ListDrivers();
void ListDataSources();
void FormatResultSet(Query& query, const tstring& rowformat);
void OutputResultSet(Query& query, const tstring& fieldseparator);

int main(int argc, char** argv) 
{
    CLI::App app{ "qx - ODBC Query eXecuter" };

    tstring connectionstring;
    tstring fieldseparator = _T("\t");
    tstring rowformat;
    tstring target = _T("stdout");
    bool verbose = false;
    bool listdrivers = false;
    bool listdsn = false;
    vector<tstring> sqlcmd;

    app.add_flag("-v,--verbose", verbose, "verbose output");
    app.add_flag("--listdrivers", listdrivers, "list installed ODBC drivers");
    app.add_flag("--listdsn", listdsn, "list ODBC data sources");
    app.add_option("-s,--source", connectionstring, "ODBC connection string");
    app.add_option("-f,--format", rowformat, "row format");
    app.add_option("--fieldseparator", fieldseparator, "fieldseparator (Default is TAB)")->excludes("--format");
    //app.add_option("-t,--target", connectionstring, "output target");
    app.add_option("sqlcmd", sqlcmd, "SQL-statement(s) (each enclosed in \"\" and space-separated)");

    try {
        app.parse(argc, argv);
    } catch(const CLI::ParseError& e) {
        return app.exit(e);
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


    // connectionstring can be specified (in order of priortity)
    // 1. as option --source on the command line
    
    // 2. in environment variable QXCONNECTION
    if (connectionstring.length() == 0)
    {
#ifdef _WIN32
        const TCHAR* cs = ::_tgetenv(_T("QXCONNECTION"));
#else
        const char* cs = std::getenv("QXCONNECTION");
#endif
        if (cs)
            connectionstring.assign(cs);
    }

    if (connectionstring.length() == 0)
    {
#ifdef _WIN32
        const TCHAR* cs = ::_tgetenv(_T("QEXCONNECTION"));
#else
        const char* cs = std::getenv("QEXCONNECTION");
#endif
        if (cs)
            connectionstring.assign(cs);
    }

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
        tcout << "Connection string: " << connectionstring << endl;
        if (rowformat.length() > 0)
            tcout << "Format: " << rowformat << endl;

        for(unsigned int i = 0; i < sqlcmd.size(); i++)
        {
            tcout << sqlcmd[i] << endl;
        }

        tcout << endl;
    }

    if (connectionstring.length() == 0 && sqlcmd.size() == 0)
        return 0;

    //rowformat = _T("[lfnbr:%5d]\\n");
    lvstring s = rowformat;
    s.Replace(_T("\\n"), _T("\n"));
    s.Replace(_T("\\t"), _T("\t"));
    rowformat = s;

    s = fieldseparator;
    s.Replace(_T("\\n"), _T("\n"));
    s.Replace(_T("\\t"), _T("\t"));
    fieldseparator = s;

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
        return nRetCode;
    } 
    catch(...) 
    {
        tcerr << _T("Cannot open connection!") << endl;
        return -1;
    }

    if (verbose)
    {
        if(b) 
            tcout << "Connection opened." << endl;
        else
            tcout << "Cannot open connection." << endl;
    }

    if(!b)
        return -1;

    // ready to execute real sql statements
    for (size_t n = 0; n < sqlcmd.size(); n++)
    {
        tstring sql = sqlcmd[n];
        if (verbose)
            tcout << sql << endl;

        try
        {
            nRetCode = query.ExecDirect(sql);

            // even a single statement (or batch of statements) can have more than one result set
            while (SQL_SUCCEEDED(nRetCode))
            {
                // Apply user-defined rowformat to each row of the result set.
                if (rowformat.length() > 0)
                {
                    ::FormatResultSet(query, rowformat);
                }

                // If there is a resultset colcount will be > 0 (even if the result holds 0 rows!)
                else if (rowformat.length() == 0)
                {
                    ::OutputResultSet(query, fieldseparator);
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
            return nRetCode;
        }

        //query.close();
    }

    if (nRetCode == SQL_NO_DATA) // end of data successfully reached
        nRetCode = SQL_SUCCESS;

   return nRetCode;
}

void FormatResultSet(Query& query, const tstring& rowformat)
{
    // ***********************************************************************
    // Iterate over the rows of the result set. if Result set has 0 rows it will 
    // skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        tcout << query.FormatCurrentRow(rowformat);
    }
}


void OutputResultSet(Query& query, const tstring& fieldseparator)
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
        tcout << fieldinfo.m_strName;
        if (col == colcount - 1)
            tcout << endl;
        else
            tcout << fieldseparator;
    }

    // ***********************************************************************
    // Now we retrieve data by iterating over the rows of the result set.
    // If Result set has 0 rows it will skip the loop because nRetCode is set to SQL_NO_DATA immediately
    // ***********************************************************************
    for (SQLRETURN nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
    {
        for (short col = 0; col < colcount; col++)
        {
            DBItem dbitem;
            query.GetFieldValue(col, dbitem);
            tcout << DBItem::ConvertToString(dbitem);
            if (col == colcount - 1)
                tcout << endl;
            else
                tcout << fieldseparator;
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

void ListDataSources()
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
