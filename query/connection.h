#pragma once

#include "tstring.h"

#include <sql.h>
#include <sqlext.h>
#include "odbcexception.h"

using namespace std;

namespace linguversa
{

class Connection
{
public:
    Connection();
    ~Connection();
    
    bool Open(tstring connectionstring);
    bool IsOpen() const;
    void Close();
    
    SQLRETURN SqlGetDriverName(tstring& drivername);
    SQLRETURN SqlGetDriverVersion(tstring& sDriverVersion); 
    
    HENV GetSqlHEnv() const { return m_henv;};
    HDBC GetSqlHDbc() const { return m_hdbc;};

protected:
    static HENV m_henv;
    static int m_ConnectionCounter;
    HDBC m_hdbc;
};

}

