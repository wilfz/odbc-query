#include <stdio.h>
#include <cassert>
#include <iostream>

#include "../query/query.h"
#include "../query/odbcenvironment.h"
#include "../query/procedure.h"

using namespace std;
using namespace linguversa;

// helper function: replace in str all occurences of from with to
void replaceAll(tstring& str, const tstring& from, const tstring& to) 
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != tstring::npos) 
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

int main(int argc, char **argv)
{
    tcout << _T("Hello ODBC!") << endl << endl;
    SQLRETURN nRetCode = SQL_SUCCESS;

    try
    {
        tcout << _T("ODBCDrivers:") << endl;
        ODBCEnvironment environment;
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
        
        tcout << endl << "ODBC data sources:" << endl;

        tstring dsn;
        tstring drivername;
        for (nRetCode = environment.FetchDataSourceInfo(dsn, drivername, SQL_FETCH_FIRST); SQL_SUCCEEDED(nRetCode); nRetCode = environment.FetchDataSourceInfo(dsn, drivername))
        {
            tcout << dsn << _T("|") << drivername <<endl;
        }
        
        cout << endl;
    }
    catch(DbException& ex)
    {
        //ex.GetErrorMessage(errmsg.GetBufferSetLength(1024),1023);
        nRetCode = ex.getSqlCode();
        tcout << _T("Cannot read list of installed drivers:") << endl << ex.what() << endl;
    }
    
    bool b = false;
    Connection con;
    try
    {
        b = con.Open(
            //_T("Driver={SQL Server};Server=localhost;Database=test;Trusted_Connection=Yes;"));
            _T("Driver={SQLite3 ODBC Driver};Database=test.db;"));// win32
            //"Driver=SQLITE3;Database=test.db;");    // un*x
    }
    catch(DbException& ex)
    {
        nRetCode = ex.getSqlCode();
        tcout << _T("Cannot open connection:") << endl;
        cout << ex.what() << endl;
        return nRetCode;
    }
    catch(...)
    {
        b = false;
    }

    if (!b)
        return -1;

    Query query;
    tstring statement = _T("drop table person;");
    try
    {
        query.SetDatabase( con);
        query.ExecDirect( statement);
    }
    catch(DbException& ex)
    {
        nRetCode = ex.getSqlCode();
        tcout << _T("Cannot drop table:") << endl << statement << endl;
        cout << ex.what() << endl;
    }
    catch(exception& ex)
    {
        tcout << _T("Cannot drop table:") << endl << statement << endl;
        cout << ex.what() << endl;
    }
    catch(...)
    {
        tcout << _T("Cannot drop table:") << endl << statement << endl;
    }
    
    try
    {
        query.SetDatabase(con);

        // ********************************************************************
        // * Executing sql statements (without result set)
        // ********************************************************************

        statement = _T("\
            create table person \
            ( \
                lfnbr                           int                   not null, \
                name                            char(32)              null    , \
                birthdte                        datetime              null    , \
                balance                         decimal(8,2)          null    , \
                pguid                           uniqueidentifier      null    , \
                constraint pk_person primary key (lfnbr) \
            ) \
            ");


        // Find out about engine and driver ...
        tstring sDriverName;
        nRetCode = con.SqlGetDriverName(sDriverName);

        // ... because postgres doesn't know data type "datetime", it has "timestamp" instead.
        if (sDriverName.find( _T("psqlodbc.dll")) < tstring::npos || sDriverName.find(_T("PSQLODBC")) < tstring::npos)
        {
            ::replaceAll( statement, _T("datetime"), _T("timestamp"));
            ::replaceAll( statement, _T("uniqueidentifier"), _T("UUID"));
            ::replaceAll( statement, _T("binary(16)"), _T("bytea"));
        }

        // ... and mysql and sqlite don't know data type "uniqueidentifier".
        if (sDriverName.find(_T("myodbc")) < tstring::npos || sDriverName.find(_T("maodbc")) < tstring::npos
            || sDriverName.find(_T("sqlite")) < tstring::npos)
        {
            ::replaceAll( statement, _T("uniqueidentifier"), _T("binary(16)")); // "char(36)");
        }

        // ********************************************************************
        // Let's start simple:
        // An SQL statement which should be executed to the database.
        // No parameters and no resultset whatsoever.
        // ********************************************************************
        nRetCode = query.ExecDirect(statement);

        // And another sql statement ...
        nRetCode = query.ExecDirect(
            _T("insert into person( lfnbr, name, birthdte, balance) values( 1, 'Jack', '1964-03-24', 5432.19)"));

        // ... and yet another.
        nRetCode = query.ExecDirect(
            _T("insert into person( lfnbr, name, birthdte, balance) values( 2, 'Jill', '2004-05-21', 1234.56)"));

        // ********************************************************************
        // Now a statements with result set
        // ********************************************************************
        nRetCode = query.ExecDirect(_T("select * from person"));

        // Check for the result set.
        short colcount = query.GetODBCFieldCount();

        // If there is a resultset colcount will be > 0 (even if the result holds 0 rows!)
        if (colcount > 0)
        {
            // ***********************************************************************
            // Optional: retrieve meta information on the columns of the result set
            // this is NOT mandatory for the subsequent retrieval of the column values
            // ***********************************************************************
            for (short col = 0; col < colcount; col++)
            {
                FieldInfo fieldinfo;
                query.GetODBCFieldInfo(col, fieldinfo);

                _tprintf(_T("columnn name:\t%s\nSQL type:\t%0d\nPrecision:\t%0d\nScale:\t%0d\nNullable:\t%0d\n\n"),
                    //							 // meta data from ODBC:
                    fieldinfo.m_strName.c_str(), //	string m_strName;
                    fieldinfo.m_nSQLType,		 //	SWORD m_nSQLType; (see "SQL data type codes" in <sql.h>
                    (int)fieldinfo.m_nPrecision, //	SQLULEN m_nPrecision;
                    fieldinfo.m_nScale,			 //	SWORD m_nScale;
                    fieldinfo.m_nNullability	 //	SWORD m_nNullability;
                );
            }

            // ***********************************************************************
            // Now we retrieve data by iterating over the rows of the result set.
            // If Result set has 0 rows it will skip the loop because nRetCode is set to SQL_NO_DATA immediately :-)
            // ***********************************************************************

            for (nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
            {
                // ***********************************************************************
                // Within this loop we use some of the various GetFieldValue() methods
                // ***********************************************************************

                // After Fetch() the current row is in the buffer.
                short col = 0;
                long lfnbr = 0;
                // retrieve an integer
                query.GetFieldValue(col++, lfnbr);	// adressing the column by its index 0

                tstring sName;
                // retrieve a string
                query.GetFieldValue(col++, sName);	// adressing the column by its index 1

                TIMESTAMP_STRUCT tsBirthDate;
                // retrieve a struct suited for date, time or combined date time columns 
                query.GetFieldValue(col++, tsBirthDate);  // adressing the column by its index 2

                double d = 0.0;
                // retrieve a double
                query.GetFieldValue(_T("balance"), d);  // adressing the column by its name

                // ********************************************************************
                // Retrieving field value of type GUID
                // ********************************************************************

                // BEGIN special hack for SQLite; for many other DBMS this can be omitted.
                // SQLite cannot recognize the SQL type and doesn't know SQL_GUID.
                // But in this special case of GUID we can cast to binary instead:
                query.SetColumnSqlType(4, SQL_BINARY);
                // END special hack for SQLite

                SQLGUID g = { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };				
                query.GetFieldValue(4, g);	// ODBC C type is implicitly set by the parameter of type GUID.
                    
                tcout << lfnbr << _T("\t") << sName << endl;
                _tprintf(_T("%02d.%02d.%04d\t"), tsBirthDate.day, tsBirthDate.month, tsBirthDate.year);
                _tprintf(_T("%8.2f\t"), d);

                // Of course fields can be NULL and be recognized as such 
                if (query.IsFieldNull(4))
                {
                    _tprintf(_T("no uniqueidentifier value stored\n"));
                }
                else
                {
                    _tprintf(_T("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"),
                        g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7] );
                }
            }
        }


        // ********************************************************************
        // * Executing sql statements with parameters
        // ********************************************************************

        // To free old bindings, always close previous query before preparing the next query statement.
        query.Close();

        // Preparing the sql statement with ? placeholders:
        // ------------------------------------------------
        query.Prepare(_T("insert into person( lfnbr, name, birthdte, balance, pguid) values( ?, ?, ?, ?, ?)"));

        ParamInfo pi;
        // Unfortunately MySQL besides not knowing datatype GUID 
        // it moreover is not able to detect datatype after Prepare().
        // Thus we tell the query the data type.
        query.GetParamInfo(5, pi);
        if (pi.m_nSQLType != SQL_GUID && pi.m_nSQLType != SQL_BINARY)
            query.SetParamType(5, SQL_C_BINARY, sizeof(SQLGUID));

        // we have the following data:
        // ---------------------------

        TIMESTAMP_STRUCT tsBirthDte1 = { 1983, 10, 17, 0, 0, 0, 0 };
        SQLGUID g1 = { 0xEC6EB6D6, 0xD42D, 0x4A6C, 0x90, 0x68, 0xF0, 0x46, 0xA8, 0x43, 0xEB, 0xA8 };
        _tprintf(_T("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"),
            g1.Data1, g1.Data2, g1.Data3, g1.Data4[0], g1.Data4[1], g1.Data4[2], g1.Data4[3], g1.Data4[4], g1.Data4[5], g1.Data4[6], g1.Data4[7]
        );

        // ********************************************************************
        // Internal parameter binding
        // --------------------------
        // The parameters are CALL-BY-VALUE.
        // The programmer need not take care about allocating and freeing 
        // memory.
        // The Query class allocates and frees the memory buffers.

        // If a statement contains N question mark placeholders the parameters are
        // numbered 1 to N. 0 is reserved for the RETCODE of the query.
        short par = 1;
        query.SetParamValue( par++, (long) 3);
        query.SetParamValue( par++, _T("Jim"));
        query.SetParamValue( par++, tsBirthDte1);
        query.SetParamValue( par++, 3523.17);
        query.SetParamValue( par++, g1);

        // Execute uses the above parameters.
        // (ExecDirect works with parameter binding, too.)
        nRetCode = query.Execute();

        assert( nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO);

        // To free old bindings, always close previous query before preparing the next query statement.
        nRetCode = query.Close();



        // Conventional parameter binding:
        // -------------------------------

        // Preparing the sql statement with ? placeholders:
        // ------------------------------------------------
        nRetCode = query.Prepare(
            _T("insert into person( lfnbr, name, birthdte, balance, pguid) values( ?, ?, ?, ?, ?)"));

        // Unfortunately MySQL besides not knowing datatype GUID 
        // it moreover is not able to detect datatype after Prepare().
        // Thus we tell the query the data type.
        query.GetParamInfo(5, pi);
        if (pi.m_nSQLType != SQL_C_GUID && pi.m_nSQLType != SQL_C_BINARY)
            query.SetParamType(5, SQL_C_BINARY, sizeof(SQLGUID));


        // we have the following data:
        // ---------------------------

        long lfnbr = 4;

        tstring sName = _T("Joe");

        TIMESTAMP_STRUCT tsBirthDte = { 2006, 7, 15, 0, 0, 0, 0 };

        double dBalance = 654.32;

        SQLGUID g2 = { 0x4B4DAFC5, 0xAE5F, 0x4342, 0x9D, 0xE7, 0x83, 0x9C, 0xFB, 0xB7, 0x63, 0x9F };
        //assert(CoCreateGuid(&g2) == S_OK);
        _tprintf(_T("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"), 
            g2.Data1, g2.Data2, g2.Data3, g2.Data4[0], g2.Data4[1], g2.Data4[2], g2.Data4[3], g2.Data4[4], g2.Data4[5], g2.Data4[6], g2.Data4[7]
        );


        // Conventional parameter binding
        // ------------------------------
        // In Conventional parameter binding the BindParameter() functions replace the 
        // question marks by host variables of various types.
        // The binding is always done by CALL-BY-REFRENCE. 
        // Since the parameters are CALL-BY-REFRENCE the bound variables (and their allocated memory)
        // have to stay alive until Close()!
        // For normal sql statements these parameters are of InputOutputType SQL_PARAM_INPUT. 
        // They stay unchanged by the execution of the sql statement.
        // When working with stored procedures parameters may be of InputOutputType SQL_PARAM_OUTPUT
        // or SQL_PARAM_INPUT_OUTPUT meaning the stored procedure call may use their value
        // and assign a new value to the host variable when executed.

        unsigned int buffersize = 32;
        TCHAR* buffer = new TCHAR[buffersize];
        // fill buffer
        for (unsigned int i = 0; i < sName.size(); i++)
        {
            if (i >= buffersize)
                break;
            buffer[i] = sName[i];
        }
        // don't forget the terminating '\0'
        unsigned int n = (unsigned int) (sName.size() < buffersize ? sName.size() : buffersize - 1);
        buffer[n] = (TCHAR)0;

        // If a statement contains N question mark placeholders the parameters are
        // numbered 1 to N. 
        // 0 is reserved for the RETCODE of the query.
        par = 1;
        query.BindParameter(par++, lfnbr);
        query.BindParameter(par++, buffer, buffersize);	// size of the corresponding field
        query.BindParameter(par++, tsBirthDte);
        query.BindParameter(par++, dBalance);
        query.BindParameter(par++, g2);

        // Execute uses the above parameters.
        // ExecDirect works with parameter binding, too.
        nRetCode = query.Execute();

        assert(nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO);

        // free resources
        // --------------
        query.Close();
        // only now we may change the bound parameters from the application level:
        delete[] buffer;

        // ********************************************************************
        // Retrieving data from the result set
        // ********************************************************************

        nRetCode = query.ExecDirect(_T("select * from person"));

        colcount = query.GetODBCFieldCount();

        // if we have a resultset colcount will be > 0 (even if the result holds 0 rows!)
        if (colcount > 0)
        {
            _tprintf(_T("\n"));

            // ***********************************************************************
            // Now we retrieve data by iterating over the rows of result set.
            // If Result set has 0 rows it will skip the loop because nRetCode is set 
            // to SQL_NO_DATA immediately :-)
            // ***********************************************************************
            for (nRetCode = query.Fetch(); nRetCode != SQL_NO_DATA; nRetCode = query.Fetch())
            {
                // ***********************************************************************
                // Within this loop we use some of the various GetFieldValue() methods.
                // ***********************************************************************

                // After Fetch() the current row is in the buffer.
                short col = 0;
                long lfnbr = 0;
                query.GetFieldValue(col++, lfnbr);	// adressing the column by its index 0

                tstring sName;
                query.GetFieldValue(_T("name"), sName);	// adressing the column by its column name

                // variant data type DBItem can be used if the column data type is unknown 
                // or no appropriate GetFieldValue() member function available.
                DBItem varBirthDte;
                query.GetFieldValue((short)2, varBirthDte);

                // Because the birthdte column is of type datetime (and not null) 
                // DBItem's type selector m_nCType is set to DBItem::lwvt_date
                // and its pointer member m_pdate is set to a valid TIMESTAMP_STRUCT
                assert(varBirthDte.m_nCType == DBItem::lwvt_date);

                // ********************************************************************
                // Retrieving field value of type decimal
                // ********************************************************************

                // balance is decimal. The default conversion goes to double
                // DBItem's type selector m_nCType is set to DBItem::lwvt_double 
                // (or DBVT_NULL if we come to a NULL value in the result set).
                short nFieldType = DEFAULT_FIELD_TYPE;

                FieldInfo fieldinfo;
                // With most databases and drivers fieldinfo.m_nSQLType will be 
                // recognized as SQL_DECIMAL and the default conversion goes to double.
                query.GetODBCFieldInfo(_T("balance"), fieldinfo);

                // BEGIN special hack for SQLite
                // But apparently sqlite driver does not recognize some SQL data 
                // types and returns -9 (Windows) or SQL_VARCHAR (unixODBC) instead.
                // Thus for this special case we have to specify nFieldType 
                // ourselves, instead of relying on the driver.
                if (fieldinfo.m_nSQLType != SQL_DECIMAL)
                    nFieldType = SQL_C_DOUBLE;	// no need to set sql type, it's sufficient to set the C type.
                // END special hack for SQLite

                // Either this way ...
                DBItem varBalance;
                // using the variant data type DBItem 
                // (nFieldType is still DEFAULT_FIELD_TYPE, except or sqlite)
                query.GetFieldValue((short)3, varBalance, nFieldType);
                assert(varBalance.m_nCType == DBItem::lwvt_double);

                // ... or the easy way:
                double d = 0.0;
                // ODBC C type is implicitly set because parameter is of type double.
                query.GetFieldValue((short)3, d);
                //varBalance = d;

                // ********************************************************************
                // Retrieving field value of type GUID
                // ********************************************************************

                // BEGIN special hack for SQLite; for many other DBMS this can be omitted.
                // SQLite cannot recognize the SQL type and doesn't know SQL_GUID.
                // But in this special case of GUID we can cast to binary instead:
                query.SetColumnSqlType(4, SQL_BINARY);
                // END special hack for SQLite

                SQLGUID g = { 0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
                query.GetFieldValue(4, g);	// ODBC C type is implicitly set by the parameter of type GUID.
                    
                tcout << lfnbr << _T("\t") << sName << endl;
                if (varBirthDte.m_pdate)
                    _tprintf(_T("%02d.%02d.%04d\t"), varBirthDte.m_pdate->day, varBirthDte.m_pdate->month, varBirthDte.m_pdate->year);
                _tprintf(_T("%8.2f\t"), d);
     
                // Of course fields can be NULL and be recognized as such 
                if (query.IsFieldNull(4))
                {
                    _tprintf(_T("no uniqueidentifier value stored\n"));
                }
                else
                {
                    _tprintf(_T("%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"),
                        g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7] );
                }
/*
                // ************************************************************
                // One of the nice features of DBItem is its ability 
                // to convert its data to a string according to a given format:
                // ************************************************************
                string sDateOfBirth = DBItem::ConvertToString(varBirthDte,
                    _T("DD.MM.YYYY"));	// e.g. continental european style

                // For data other than TIMESTAMP_STRUCT, simply use the well-known c format specifiers:
                string sFormattedBalance = DBItem::ConvertToString(varBalance,
                    _T("%8,2f"));
                // The comma in the above format specification is not an error!!!
                // Other than in standard C++, DBItem and LwQuery members allow for the 
                // european decimal separator, instead of the american, and convert 
                // according to the decimal separator used in the format specification.

                _tprintf(_T("Nr. %0d\t%s\t%s\t%s\t%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n"), lfnbr, (LPCTSTR)sName,
                    (LPCTSTR)sDateOfBirth, (LPCTSTR)sFormattedBalance,
                    g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]
                    );
*/
            }
        }

        // free resources
        // --------------
        nRetCode = query.Close();

        // ********************************************************************
        // * Working with stored procedures
        // ********************************************************************

        // So far I have only a testproc for SQL Server, MySQL, and MariaDB
        if (sDriverName.find(_T("SQLSRV")) >= string::npos && sDriverName.find(_T("sqlncli")) >= string::npos
            && sDriverName.find(_T("myodbc")) >= string::npos && sDriverName.find(_T("maodbc")) >= string::npos)
        {
            // none of the above found
            return nRetCode;
        }
        
        // This little stored procedure demonstrates the use of output parameters.
        // It simply adds the value of of the first to the second parameter
        // and assigns the concatenation of the input strings in the third and
        // fourth parameter to the fourth parameter.
        nRetCode = query.Prepare(_T("{CALL testproc(?,?,?,?)}"));

        // internal parameter binding
        // --------------------------
        par = 1;
        query.SetParamValue( 1, (long) 3);
        query.SetParamValue( 2, (long) 5, ParamInfo::inputoutput);	// stored procedure changes this parameter
        query.SetParamValue( 3, _T("ABC"));
        query.SetParamValue( 4, _T("DEF"), ParamInfo::inputoutput);	// stored procedure changes this parameter

        // Execute uses the above parameters.
        nRetCode = query.Execute();

        assert( nRetCode == SQL_SUCCESS || nRetCode == SQL_SUCCESS_WITH_INFO);

        cout << endl;

        // ********************************************************************
        // * Getting values of inputouput parameters
        // ********************************************************************

        // Most ODBC drivers set the output parameters automatically after
        // the last result set has been completely read. 
        // The function Finish() calls SQLMoreResults() until every result
        // set is finished.
        //
        // MySQL's ODBC driver in contrast returns the values for output 
        // parameters (if there are any at all) as an additional single row 
        // result set.
        // For this driver Finish() proceeds to the very last result set,  
        // reads its one and only row and copies its values into the output 
        // and inout parameters' buffers. 
        nRetCode = query.Finish();
        // 
        // end of hack for MySQL's ODBC driver

        long lVal = 0;
        tstring sVal;

        query.GetParamValue( 2, lVal);	
        _tprintf(_T("lVal = %0ld\n"), lVal);	// --> lVal = 8

        query.GetParamValue( 4, sVal);
        _tprintf(_T("sVal = %s\n"), sVal.c_str());	// --> sVal = "ABCDEF"

    }
    catch(DbException& ex)
    {
        nRetCode = ex.getSqlCode();
        tcout << _T("Runtime error:") << endl;
        cout << ex.what() << endl;
    }

    // free resources
    // --------------
    int nRetCode2 = query.Close();

    con.Close();

    return nRetCode ? nRetCode : nRetCode2;
}
