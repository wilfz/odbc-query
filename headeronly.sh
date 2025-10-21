#!/usr/bin/env bash
mkdir headeronly 
cd query
cat tstring.h std_includes.h > ../headeronly/odbcquery.hpp
cat odbcexception.h connection.h dbitem.h fieldinfo.h resultinfo.h datarow.h paraminfo.h paramitem.h query.h table.h lvstring.h odbcenvironment.h connection.cpp dbitem.cpp fieldinfo.cpp resultinfo.cpp datarow.cpp paramitem.cpp lvstring.cpp query.cpp odbcexception.cpp odbcenvironment.cpp table.cpp | grep -iv "#include" | grep -iv "#pragma once" >> ../headeronly/odbcquery.hpp
cd ..
