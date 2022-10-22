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

#include "tstring.h"
#include <vector>
#include "dbitem.h"

#define SQL_C_STRING SQL_VARCHAR

using namespace std;

namespace linguversa
{
class LwDataRow : public vector<DBItem>
{
public:
    //LwDataRow();
    //LwDataRow( const LwDataRow& src);
    //const LwDataRow& operator = ( const LwDataRow& src);
    //bool operator == (const LwDataRow& other) const;
    //bool operator != (const LwDataRow& other) const;

    //tstring Format( const LwResultInfo& fieldinfo, LPCTSTR fmt = NULL) const;

protected:
    //LwResultInfo* m_pResultInfo;

    //friend LW_EXT_CLASS CArchive& operator <<(CArchive& ar, const LwDataRow& row);
    //friend LW_EXT_CLASS CArchive& operator >>(CArchive& ar, LwDataRow& row);
};

//LW_EXT_CLASS CArchive& operator <<(CArchive& ar, const LwDataRow& row);
//LW_EXT_CLASS CArchive& operator >>(CArchive& ar, LwDataRow& row);

}
