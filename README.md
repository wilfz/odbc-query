# odbc-query
Command line tool **qx** and library **odbcquery** to easily access databases by ODBC in C++<br>
Multiplatform: Works for Windows and Linux.<br>
## Motivation
ODBC provides an unified access to most database systems on many platforms, including Windows and Linux. But the ODBC API was written in C at a time when C++ was still the new kid on the block.
So it can be confusing, time-consuming and error-prone to use ODBC directly in C++ projects.
## Features
Although there are already some ODBC wrappers for C++ I decided to create this Library because I needed some features I didn't find elsewhere:
- simple usage
- Windows and linux compatibilty
- Windows support for x86 and x64, ANSI-Code / Multibyte and UNICODE.
- support of data definition statements, data query statements, data manipulation statements
- parameterized statements to avoid SQL injection
- support of stored procedures
- retrieval of output parameters
- access to meta data of parameters, result columns and tables
- ANSI/Multibyte and Windows UNICODE

The purpose of this library is neither to cover the complete potential of ODBC, nor to keep programmers away from the underlying API. Instead it should provide an easy way for the most common database-related tasks and give C++ programmers a guideline on how to use the more advanced and specific features of ODBC.  
## Prerequistes
To compile and use on linux-like systems it may be necessary to install unixodbc first:
```bash
sudo apt-get install unixodbc
sudo apt-get install unixodbc-dev
```
The demo application uses Christian Werner's excellent [sqliteodbc driver](http://www.ch-werner.de/sqliteodbc/).
For Windows just run [sqliteodbc.exe](http://www.ch-werner.de/sqliteodbc/sqliteodbc.exe) and/or [sqliteodbc_w64.exe](http://www.ch-werner.de/sqliteodbc/sqliteodbc_w64.exe) to intall.<br>
For Linux install the drivers with:
```bash
sudo apt-get install sqlite3
sudo apt-get install libsqliteodbc
```
## How to use the odbcquery library
The best way to understand the library is to step through the source *main.cpp* of the **demo** application. I will simplify the code here and not distinguish slight differences because of character sets, operating systems.

At first it is necessary to establish a connection to a data source:
```c++
SQLRETURN nRetCode = SQL_SUCCESS;
bool b = false;
Connection con;
try
{
    b = con.Open(
      "Driver={SQLite3 ODBC Driver};Database=test.db3;");
}
catch(DbException& ex)
{
    nRetCode = ex.getSqlCode();
    ...
}
```
In this example we intantiate a **Connection** object con. The connection string in the Open funcion specifies as data source a SQLite database, which resides in your current working directory of your local file system. That has the advantage that we can play around with the data without tampering real data in a production environment. With other connection strings you may connect to large dstributed database systems.

Besides Connection we see another library class: **DBException**. Of course it inherits from exception.

The library classes throw a DBException whenever some unexpected event interrupts the normal flow of the program, for instance when the connection to the database is broken or when we encounter an syntax error in the execution of an SQL command etc. The member function getSqlCode() returns the SQLRETURN value of the API function that triggered the exception.

When running the demo program for the first time it generates such an exception in the next code sequence:
```c++
Query query;
string statement = "drop table person;";
try
{
    query.SetDatabase( con);
    query.ExecDirect( statement);
}
catch(DbException& ex)
{
    nRetCode = ex.getSqlCode();
    cout << "Cannot drop table:" << endl;
    cout << ex.what() << endl;
}
```
As its name suggests the **Query** class is the central class of the odbcquery library. It is used to execute any SQL satement, not only data queries, but also statements of the data definition language such as "create table" or "drop table" and data manipulation statements such as "insert" or "update". You may see the class name **Query** as a reference to the middle part of "Structured Query Language".

Of course executing the statement "drop table person;" in a newly created database causes an exception, since there is no such table.

For the sake of an example program this nevertheless makes sense, since we may want to run the program a second and a third time.

In that case we would get an DBException with the next code squence.
