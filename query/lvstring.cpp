#include "lvstring.h"

using namespace std;

namespace linguversa
{
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

tstring lvstring::Left(const unsigned int n) const
{
    return tstring::substr( 0, n);
}

tstring lvstring::Right(const unsigned int n) const
{
    unsigned int cnt = tstring::length();
    if (n < cnt)
        return tstring::substr( cnt - n, n);
    else 
        return tstring::substr( 0, tstring::npos);
}

tstring lvstring::Mid(const unsigned int from, const unsigned int cnt) const
{
    return tstring::substr( from, cnt);
}

}
