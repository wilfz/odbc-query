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
#include <vector>

#define SQL_C_STRING SQL_VARCHAR

using namespace std;

namespace linguversa
{
    typedef vector<unsigned char> bytearray;

    class DBItem
    {
    public:
        DBItem();
        DBItem( const DBItem& src);
        virtual ~DBItem();

        typedef enum {
            lwvt_null,    // DBVT_NULL =  0
            lwvt_bool,    // DBVT_BOOL,
            lwvt_uchar,   // DBVT_UCHAR,
            lwvt_short,   // DBVT_SHORT,
            lwvt_long,    // DBVT_LONG,
            lwvt_single,  // DBVT_SINGLE,
            lwvt_double,  // DBVT_DOUBLE,
            lwvt_date,    // DBVT_DATE,
            lwvt_string,  // DBVT_STRING,
            lwvt_binary,  // DBVT_BINARY,
            lwvt_astring, // DBVT_ASTRING,
            lwvt_wstring, // DBVT_WSTRING, = 11
            lwvt_bytearray = 24, // LWVT_BYTEARRAY = 24,
            lwvt_uint64 = 25, // LWVT_UINT64 = 25;
            lwvt_guid = 27,   // LWVT_GUID   = 27,
        } vartype;
        
        void clear();
        DBItem& operator = ( const DBItem& src);

        bool operator == (const DBItem& other) const;

        vartype m_nCType;
        
        union
        {
          bool              m_boolVal;
          unsigned char     m_chVal;    // SQL_C_UTINYINT  = SQL_TINYINT + SQL_UNSIGNED_OFFSET = -28 /* BYTE   */
          short             m_iVal;     // SQL_C_SHORT     = SQL_SMALLINT := 5   /* SMALLINT      */
          long              m_lVal;     // SQL_C_LONG      = SQL_INTEGER := 4    /* INTEGER       */
          float             m_fltVal;   // SQL_C_FLOAT     = SQL_REAL := 7       /* REAL          */
          double            m_dblVal;   // SQL_C_DOUBLE    = SQL_DOUBLE := 8     /* FLOAT, DOUBLE */
          TIMESTAMP_STRUCT* m_pdate;    // SQL_C_TIMESTAMP = SQL_TIMESTAMP := 11 /* DATETIME      */
          string*           m_pstring;  // SQL_C_CHAR      = SQL_CHAR := 1       /* CHAR, VARCHAR */
          bytearray*        m_pByteArray; // SQL_C_BINARY  = SQL_BINARY := -2
          unsigned ODBCINT64* m_pUInt64;  // SQL_C_UBIGINT = SQL_BIGINT + SQL_UNSIGNED_OFFSET = -27
          SQLGUID*          m_pGUID;    // SQL_C_GUID :    = SQL_GUID = -11
        };
        
    protected:
        void copyfrom( const DBItem& src);
    };

}
