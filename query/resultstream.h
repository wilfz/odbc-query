#pragma once

#include <iostream>
#include "dbitem.h"
#include "datarow.h"
#include "resultinfo.h"

namespace linguversa
{
	std::ostream& operator <<(std::ostream& ar, const DBItem& item);
	std::istream& operator >>(std::istream& ar, DBItem& item);

	std::ostream& operator <<(std::ostream& ar, const DataRow& row);
	std::istream& operator >>(std::istream& ar, DataRow& row);

	std::ostream& operator <<(std::ostream& ar, const FieldInfo& info);
	std::istream& operator >>(std::istream& ar, FieldInfo& info);

	std::ostream& operator <<(std::ostream& ar, const ResultInfo& resultinfo);
	std::istream& operator >>(std::istream& ar, FieldInfo& resultinfo);
}