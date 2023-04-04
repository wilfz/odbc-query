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

using namespace std;
using namespace linguversa;
using namespace CLI;

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
    app.add_option("--fieldseparator", fieldseparator, "fieldseparator")->excludes("--format");
    //app.add_option("-t,--target", connectionstring, "output target");
    app.add_option("sqlcmd", sqlcmd, "SQL-statement(s) (each enclosed in \"\" and space-separated)");

    try {
        app.parse(argc, argv);
    } catch(const CLI::ParseError& e) {
        return app.exit(e);
    }

    // first get information independent of specific connection
    ODBCEnvironment environment;
    SQLRETURN nRetCode = SQL_SUCCESS;

    try 
    {
        if (listdrivers)
        {
            tcout << _T("ODBCDrivers:") << endl;
            tstring driver;
            vector<tstring> attributes;
            for (nRetCode = environment.FetchDriverInfo(driver, attributes, SQL_FETCH_FIRST); 
                SQL_SUCCEEDED(nRetCode);
                nRetCode = environment.FetchDriverInfo(driver, attributes)) 
            {
                tcout << driver << endl;
                for (unsigned short i = 0; i < attributes.size(); i++)
                    tcout << _T("\t") << attributes[i] << endl;
            }

            tcout << endl;
        }

        if (listdsn) 
        {
            tcout << "ODBC data sources:" << endl;

            tstring dsn;
            tstring drivername;
            for (nRetCode = environment.FetchDataSourceInfo(dsn, drivername, SQL_FETCH_FIRST); SQL_SUCCEEDED(nRetCode);
                nRetCode = environment.FetchDataSourceInfo(dsn, drivername)) 
            {
                tcout << dsn << _T("|") << drivername << endl;
            }

            tcout << endl;
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

        for(unsigned int i = 1; i < sqlcmd.size(); i++)
        {
            tcout << sqlcmd[i] << endl;
        }
        tcout << endl;
    }

    if (connectionstring.length() == 0 && sqlcmd.size() == 0)
        return 0;

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
        try
        {
            if (verbose)
                tcout << sql << endl;

            nRetCode = query.ExecDirect(sql);

            // Check for the result set.
            short colcount = query.GetODBCFieldCount();

            // If there is a resultset colcount will be > 0 (even if the result holds 0 rows!)
            while (colcount > 0 && rowformat.length() == 0)
            {
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
                for (nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
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

                nRetCode = query.SQLMoreResults();
                if (SQL_SUCCEEDED(nRetCode))
                    colcount = query.GetODBCFieldCount();
                else
                    colcount = 0; // or break;
            }
        }
        catch (DbException& ex)
        {
            nRetCode = ex.getSqlCode();
            tcerr << _T("Execute error:") << endl;
            cerr << ex.what() << endl;
            return nRetCode;
        }
    }

    if (nRetCode == SQL_NO_DATA) // end of data successfully reached
        nRetCode = SQL_SUCCESS;

   return nRetCode;
}
