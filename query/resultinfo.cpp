#include "resultinfo.h"

using namespace std;
using namespace linguversa;

int ResultInfo::GetSqlColumn(tstring colname) const
{
	if (colname.length() == 0)
		return -1;

	for (unsigned int sqlcol = 0; sqlcol < vector<FieldInfo>::size(); sqlcol++)
	{
		if ((*this)[sqlcol].m_strName == colname) // TODO CompareNoCase
			return sqlcol;
	}

	return -1;
}

