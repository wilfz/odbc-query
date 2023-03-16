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
    tstring target;
    bool verbose = false;
    bool listdrivers = false;
    bool listdsn = false;
    vector<tstring> sqlcmd;

    app.add_flag("--listdrivers", listdrivers, "list installed drivers");
    app.add_flag("--listdsn", listdsn, "List data sources");
    app.add_option("-s,--source", connectionstring, "ODBC connection string");
    //app.add_option("-t,--target", connectionstring, "output target");
    app.add_flag("-v,--verbose", verbose, "verbose output");
    app.add_option("sqlcmd", sqlcmd, "SQL statement");

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
        const char* cs = std::getenv("QXCONNECTION");
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
            const char* cs = ini.GetValue("Database", "ConnectionString", "");
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

    bool b = false;
    Connection con;
    try 
    {
        b = con.Open( connectionstring);
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

    Query query;
}
