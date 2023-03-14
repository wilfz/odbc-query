#pragma once

#include "tstring.h"
#include <vector>
#include "dbitem.h"
#include "resultinfo.h"

#define SQL_C_STRING SQL_VARCHAR

namespace linguversa
{
class DataRow : public std::vector<DBItem>
{
public:
    //LwDataRow();
    //LwDataRow( const LwDataRow& src);
    //const LwDataRow& operator = ( const LwDataRow& src);
    //bool operator == (const LwDataRow& other) const;
    //bool operator != (const LwDataRow& other) const;

    std::tstring Format( const ResultInfo& resultinfo, const std::tstring fmt = "") const;

protected:
    //ResultInfo* m_pResultInfo;

    //friend LW_EXT_CLASS CArchive& operator <<(CArchive& ar, const LwDataRow& row);
    //friend LW_EXT_CLASS CArchive& operator >>(CArchive& ar, LwDataRow& row);
};

//LW_EXT_CLASS CArchive& operator <<(CArchive& ar, const LwDataRow& row);
//LW_EXT_CLASS CArchive& operator >>(CArchive& ar, LwDataRow& row);

}
