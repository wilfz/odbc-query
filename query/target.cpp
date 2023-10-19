#include "target.h"
#include "query.h"
#include <cassert>

using namespace std;
using namespace linguversa;

Target::Target()
{
	selector = sel_stdout;
}

Target::~Target()
{
	Close();
}

bool Target::OpenConnection(std::tstring connection)
{
	Close();
	bool ret = m_connnection.Open(connection);
	selector = ret ? sel_odbc : sel_stdout;
	return ret;
}

bool Target::OpenFile(std::tstring filepath)
{
	Close();
	try {
		m_outstream.open(filepath);
	}
	catch (...) {
		selector = sel_stdout;
		return false;
	}

	if (m_outstream.is_open())
	{
		selector = sel_fileout;
		return true;
	}
	else
	{
		selector = sel_stdout;
		return false;
	}
}

bool Target::IsOpen()
{
	return m_outstream.is_open() || m_connnection.IsOpen();
}

void Target::Close()
{
	m_outstream.close();
	m_connnection.Close();
	selector = sel_stdout;
}

bool Target::Apply(std::tstring str)
{
	bool ret = false;
	if (selector == sel_odbc && m_connnection.IsOpen())
	{
		Query query(&m_connnection);
		ret = query.ExecDirect(str);
	}
	else if (selector == sel_fileout || selector == sel_stdout)
	{
		// output stream os, may either be the just opened file or otherwise stdout
		tostream& os = selector == sel_fileout && m_outstream.is_open() ? m_outstream : tcout;
		os << str;
		ret = true;
	}
	return ret;
}

