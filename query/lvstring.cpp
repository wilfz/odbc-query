#include "lvstring.h"
#include <sql.h>

using namespace std;
using namespace linguversa;

void lvstring::Replace(const tstring from, const tstring to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = tstring::find(from, start_pos)) != tstring::npos) 
    {
        tstring::replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

tstring lvstring::Left(const size_t n) const
{
    return tstring::substr( 0, n);
}

tstring lvstring::Right(const size_t n) const
{
    size_t cnt = tstring::length();
    if (n < cnt)
        return tstring::substr( cnt - n, n);
    else 
        return tstring::substr( 0, tstring::npos);
}

tstring lvstring::Mid(const size_t from, const size_t cnt) const
{
    return tstring::substr( from, cnt);
}

tstring& lvstring::MakeLower()
{
    for (size_t i = 0; i < length(); i++)
    {
        TCHAR& c = at(i);
        if (c >= 'A' && c <= 'Z')
            c += 32;
    }

    return *this;
}

tstring& lvstring::MakeUpper()
{
    for (size_t i = 0; i < length(); i++)
    {
        TCHAR& c = at(i);
        if (c >= 'a' && c <= 'z')
            c -= 32;
    }

    return *this;
}

tstring linguversa::lower(const std::tstring& str)
{
    lvstring ret = str;
    return ret.MakeLower();
}

tstring linguversa::upper(const std::tstring& str)
{
    lvstring ret = str;
    return ret.MakeUpper();
}

tstring linguversa::FormatTimeStamp(const tstring& colFmt, const TIMESTAMP_STRUCT* pdt)
{
    // The value of the fraction field is the number of billionths of a second and ranges from 0 through 999999999 (1 less than 1 billion).
    // For example, the value of the fraction field for a half - second is 500000000, for a thousandth of a second(one millisecond) is 1000000, 
    // for a millionth of a second(one microsecond) is 1000, and for a billionth of a second(one nanosecond) is 1.
    lvstring sValue = colFmt;
    if (colFmt.length()==0)
    {
        if (pdt)
        {
            sValue = linguversa::string_format(_T("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
                pdt->year, pdt->month, pdt->day,
                pdt->hour, pdt->minute, pdt->second, pdt->fraction / 1000000);
        }
        else
        {
            sValue = _T("0000-00-00 00:00:00.000");
        }
    }
    else
    {
        // Replace the placholder for pdt->fraction first, otherwise we 
        // might replace an already valid part of the date representation.
        lvstring sFraction;
        sFraction = linguversa::string_format(_T(".%09d"), pdt ? pdt->fraction : 0);
        // TODO:
        //if (sFraction.GetLength() > 10)
        //  throw exception!
        sValue.Replace(_T(".999999999"), sFraction.Left(10));
        sValue.Replace(_T(".99999999"), sFraction.Left(9));
        sValue.Replace(_T(".9999999"), sFraction.Left(8));
        sValue.Replace(_T(".999999"), sFraction.Left(7));
        sValue.Replace(_T(".99999"), sFraction.Left(6));
        sValue.Replace(_T(".9999"), sFraction.Left(5));
        sValue.Replace(_T(".999"), sFraction.Left(4));
        sValue.Replace(_T(".99"), sFraction.Left(3));
        sValue.Replace(_T(".9"), sFraction.Left(2));

        lvstring sYear;
        sYear = linguversa::string_format(_T("%04d"), pdt ? pdt->year : 0);
        sValue.Replace(_T("YYYY"), sYear);
        sYear = linguversa::string_format(_T("%02d"), pdt ? pdt->year : 0);
        sValue.Replace(_T("YY"), sYear.Right(2));
        lvstring sMonth;
        sMonth = linguversa::string_format(_T("%02d"), pdt ? pdt->month : 0);
        sValue.Replace(_T("MM"), sMonth);
        sMonth = linguversa::string_format(_T("%01d"), pdt ? pdt->month : 0);
        sValue.Replace(_T("M"), sMonth);
        lvstring sDay;
        sDay = linguversa::string_format(_T("%02d"), pdt ? pdt->day : 0);
        sValue.Replace(_T("DD"), sDay);
        sDay = linguversa::string_format(_T("%01d"), pdt ? pdt->day : 0);
        sValue.Replace(_T("D"), sDay);
        lvstring sHour;
        sHour = linguversa::string_format(_T("%02d"), pdt ? pdt->hour : 0);
        sValue.Replace(_T("hh"), sHour);
        sHour = linguversa::string_format(_T("%01d"), pdt ? pdt->hour : 0);
        sValue.Replace(_T("h"), sHour);
        lvstring sMinute;
        sMinute = linguversa::string_format(_T("%02d"), pdt ? pdt->minute : 0);
        sValue.Replace(_T("mm"), sMinute);
        lvstring sSecond;
        sSecond = linguversa::string_format(_T("%02d"), pdt ? pdt->second : 0);
        sValue.Replace(_T("ss"), sSecond);
    }

    return (tstring)sValue;
}

// helper function
static tstring StrLenFormat(tstring str, tstring fmt)
{
    size_t t = fmt.find(_T(","));
    tstring sLen = (t != tstring::npos) ? fmt.substr(0, t) : fmt;
    size_t minlen = ::_tstoi(sLen.c_str());
    size_t maxlen = (t != tstring::npos && t + 1 < fmt.length()) ? ::_tstoi(fmt.substr(t + 1).c_str()) : minlen;
    size_t curlen = str.length();
    if (curlen < minlen && minlen <= maxlen)
        str.resize(minlen);
    for (size_t i = curlen; i < minlen; i++)
        str[i] = _T(' ');
    if (minlen <= maxlen && maxlen > 1 && maxlen < str.length())
        return str.substr(0,maxlen);
    else
        return str;
}

bool ConvertNationalSeparator(tstring& format)
{
    if (format.length()==0)
        format = _T("%01.6f");
    size_t posNationalDecSep = format.find(_T(","), 0);
    size_t posCDecSep = format.find(_T("."), 0);
    bool bNationalDecSep = (posNationalDecSep != tstring::npos && posCDecSep == tstring::npos);
    if (bNationalDecSep)
    {
        lvstring f = format;
        f.Replace(_T(","), _T("."));
        format = f;
    }
    return bNationalDecSep;
}
