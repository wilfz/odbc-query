#pragma once

#include <iostream>
#include "dbitem.h"
#include "datarow.h"
#include "resultinfo.h"

namespace linguversa
{
	std::tostream& operator <<(std::tostream& ar, const DBItem& item);
	std::tistream& operator >>(std::tistream& ar, DBItem& item);

	std::tostream& operator <<(std::tostream& ar, const DataRow& row);
	std::tistream& operator >>(std::tistream& ar, DataRow& row);

	std::tostream& operator <<(std::tostream& ar, const FieldInfo& info);
	std::tistream& operator >>(std::tistream& ar, FieldInfo& info);

	std::tostream& operator <<(std::tostream& ar, const ResultInfo& resultinfo);
	std::tistream& operator >>(std::tistream& ar, ResultInfo& resultinfo);
}