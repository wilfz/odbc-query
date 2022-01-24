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
#include <string>

using namespace std;

namespace linguversa
{
    // Element of Implementation Parameter Descriptor (IPD):
    // Contains information about the parameter in an SQL statement..
    class ParamInfo
    {
    public:
        // members of CODBCParamInfo
        SWORD m_nSQLType;
        UDWORD m_nPrecision;
        SWORD m_nScale;
        SWORD m_nNullability;

        typedef enum {
            unknown = SQL_PARAM_TYPE_UNKNOWN,     //  0
            input = SQL_PARAM_INPUT,              //  1
            inputoutput = SQL_PARAM_INPUT_OUTPUT, //  2
            resultcoumn = SQL_RESULT_COL,         //  3
            output = SQL_PARAM_OUTPUT,            //  4
            returnvalue = SQL_RETURN_VALUE        //  5
        } InputOutputType;

        // other members
        InputOutputType m_InputOutputType;
        // nur für stored proceduers
        string m_ParamName;

        ParamInfo()
        {
            m_nSQLType = SQL_UNKNOWN_TYPE;
            m_nPrecision = 0;
            m_nScale = 0;
            m_nNullability = 0;
            m_InputOutputType = unknown;
        }
    };
    
}
