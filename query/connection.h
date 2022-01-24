#pragma once

#ifdef _WIN32
// needs to be included above sql.h for windows
#ifndef __MINGW32__
#define NOMINMAX
#if _MSC_VER <= 1500
#define nullptr NULL
#endif // _MSC_VER <= 1500
#endif
#include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include "odbcexception.h"
#include <string>

using namespace std;

namespace linguversa
{

class Connection
{
public:
    Connection();
    ~Connection();
    
    bool Open(string connectionstring);
    bool IsOpen() const;
    void Close();
    
    SQLRETURN SqlGetDriverName(string& drivername);
    SQLRETURN SqlGetDriverVersion(string& sDriverVersion); 
    
    HENV GetSqlHEnv() const { return m_henv;};
    HDBC GetSqlHDbc() const { return m_hdbc;};

protected:
    static HENV m_henv;
    static int m_ConnectionCounter;
    HDBC m_hdbc;
};

}

