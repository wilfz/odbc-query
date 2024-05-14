#pragma once

#include "query.h"
#include <fstream>
#include <iostream>
#include <sstream>
#ifdef _WIN32
#ifdef UNICODE
#define tcout wcout
#define tostream wostream
#define tofstream wofstream
#define tstringstream wstringstream
#else
#define tcout cout
#define tostream ostream
#define tofstream ofstream
#define tstringstream stringstream
#endif
#else
#define tcout cout
#define tostream ostream
#define tofstream ofstream
#define tstringstream stringstream
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

    class TargetStream : public std::tostream
    {
    public:
    	// default constructor
        TargetStream() : std::tostream(nullptr) {};
        TargetStream(std::streambuf* pbuf) : std::tostream(pbuf) { };
        // TODO:
        TargetStream( linguversa::Connection& con);

    };
}
