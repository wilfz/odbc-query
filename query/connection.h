#pragma once

#include "tstring.h"

#include <sql.h>
#include <sqlext.h>
#include "odbcexception.h"

namespace linguversa
{

class Connection
{
public:
    Connection();
    ~Connection();
    
    bool Open(std::tstring connectionstring);
    bool IsOpen() const;
    void Close();
    
    SQLRETURN SqlGetDriverName(std::tstring& drivername);
    SQLRETURN SqlGetDriverVersion(std::tstring& sDriverVersion); 
    
    HENV GetSqlHEnv() const { return m_henv;};
    HDBC GetSqlHDbc() const { return m_hdbc;};

protected:
    static HENV m_henv;
    static int m_ConnectionCounter;
    HDBC m_hdbc;
};

}

