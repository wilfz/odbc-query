#!/usr/bin/env sh
cat odbcexception.h connection.h dbitem.h fieldinfo.h resultinfo.h datarow.h paraminfo.h paramitem.h query.h table.h lvstring.h odbcenvironment.h connection.cpp dbitem.cpp fieldinfo.cpp resultinfo.cpp datarow.cpp paramitem.cpp lvstring.cpp query.cpp odbcexception.cpp odbcenvironment.cpp table.cpp | grep -iv "#include" | grep -iv "#pragma once" >../single_header/tmp.txt
cat tstring.h ../single_header/std_includes.txt ../single_header/tmp.txt > ../single_header/odbcquery.hpp
rm ../single_header/tmp.txt
