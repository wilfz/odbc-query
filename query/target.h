#pragma once

#include "connection.h"
#include <fstream>
#include <iostream>
#ifdef _WIN32
#ifdef UNICODE
#define tcout wcout
#define tostream wostream
#define tofstream wofstream
#else
#define tcout cout
#define tostream ostream
#define tostream ostream
#endif
#else
#define tcout cout
#define tostream ostream
#define tofstream ofstream
#endif


namespace linguversa
{
    class ResultGroup
    {
    protected:
        uint64_t selector;
        std::tstring firstRowFormat;
        std::tstring eachRowFormat;
        std::tstring lastRowFormat;
        //DataRow aggregation;
    };

    class Target
    {
    public:
        Target();
        ~Target();

        bool OpenConnection(std::tstring connection);
        bool OpenFile(std::tstring filepath);
        bool IsOpen();
        void Close();
        bool Apply(std::tstring str);

    protected:
        enum {
            sel_stdout,
            sel_fileout,
            sel_odbc
        } selector;
        std::tofstream m_outstream;
        linguversa::Connection m_connnection;
    };
}
