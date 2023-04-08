#include "datarow.h"
#include "lvstring.h"

using namespace std;
using namespace linguversa;

std::tstring DataRow::Format(const ResultInfo& resultinfo, const std::tstring fmt) const
{
	// Parse the format string and if we find a pair of [] check the content against the coiumn names.
	// If we find a matching column name, replace the content of the brackets according to the format.
	// Continue parsing behind the replaced content or at the next [ if not match was found. 
	lvstring retVal;
	retVal = fmt;
	lvstring colname;
	tstring sValue;

	size_t pos0 = retVal.find(_T("["));
	while (pos0 != tstring::npos)
	{
		size_t pos1 = retVal.find(_T(":"), pos0);
		size_t pos2 = retVal.find(_T("]"), pos0);
		if (pos2 == tstring::npos)
			break; // we are done, nothing more to replace
		tstring colfmt;
		
		if (pos1 != tstring::npos && pos1 < pos2)
		{
			// ':' and format found
			colname = retVal.substr(pos0 + 1, pos1 - pos0 - 1);
			colfmt = retVal.substr(pos1 + 1, pos2 - pos1 - 1);
		}
		else
		{
			// no format found
			colname = retVal.substr(pos0 + 1, pos2 - pos0 - 1);
		}

		int col = -1;
		// does colname match any column name in resulltinfo?
		if (colname.length() > 0 && (col = resultinfo.GetSqlColumn(colname)) >= 0)
		{
			const DBItem& var = at(col);
			sValue = DBItem::ConvertToString(var, colfmt);
			retVal.replace(pos0, pos2 - pos0 + 1, sValue);
			pos0 = retVal.find(_T("["), pos0 + sValue.length());
		}
		else
		{
			// search for the next occurence of [
			pos0 = retVal.find(_T("["), pos0 + 1);
		}
	}


	return retVal;
}
