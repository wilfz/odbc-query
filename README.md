# odbc-query
Command line tool **qx** and library **odbcquery** to easily access databases by ODBC in C++<br>
Multiplatform: Works for Windows and Linux.<br>
Windows support for x86 and x64, Multibyte and UNICODE.<br>
## Prerequistes
To compile and use on linux-like system it may be necessary to install unixodbc first:
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
