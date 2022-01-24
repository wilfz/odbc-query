#include "paramitem.h"
#include <cassert>

using namespace linguversa;
using namespace std;

ParamItem::ParamItem()
{
    m_nParamLen = 0;
    m_nCType = SQL_TYPE_NULL;
    m_pParam = NULL;
    m_lenInd = 0;
    m_local = false;
}

ParamItem::~ParamItem()
{
    Clear();
}

void ParamItem::Clear()
{
    if (!m_local)
        return;
    assert(m_local);
    if (m_pParam != NULL)
    {
        switch (m_nCType)
        {
        case SQL_C_LONG:
            delete (long*)m_pParam;
            break;
        case SQL_C_DOUBLE:
            delete (double*)m_pParam;
            break;
        case SQL_C_TIMESTAMP:
            delete (TIMESTAMP_STRUCT*)m_pParam;
            break;
        case SQL_C_CHAR:
        case SQL_C_WCHAR:
            delete[]((TCHAR*)m_pParam);
            break;
        case SQL_C_BINARY:
            delete[]((BYTE*)m_pParam);
            break;
        case SQL_C_GUID:
            delete (SQLGUID*)m_pParam;
            break;
        default:
            assert(false);
        }
        m_pParam = NULL;
        m_nCType = 0;

        // Prevent from deleting again unless m_local is explicitly set.
        m_local = false;

        // don't delete driver side information
    }
}

