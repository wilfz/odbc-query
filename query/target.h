#pragma once

#include "query.h"
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
#define tofstream ofstream
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

        typedef enum {
            sel_stdout,
            sel_fileout,
            sel_odbc
        } selector_type;

        bool OpenStdOutput();
        bool OpenConnection(std::tstring connection);
        bool OpenFile(std::tstring filepath);
        bool IsOpen();
        void Close();

        bool Apply(std::tstring str);
        void Create( const Query& query, tstring tablename);
        void InsertCurrentRow(Query& query, tstring tablename);
        void InsertAll(Query&, tstring tablename);
        void OutputAsCSV(Query& query, const tstring& fieldseparator);

    protected:
        selector_type selector;
        std::tofstream m_outstream;
        linguversa::Connection m_connnection;
    };
}
