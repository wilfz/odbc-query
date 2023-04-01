set(libSrcs 
    connection.h
    connection.cpp
    query.h
    query.cpp
    datarow.h
    datarow.cpp
    dbitem.h
    dbitem.cpp
    fieldinfo.h
    fieldinfo.cpp
    odbcenvironment.h
    odbcenvironment.cpp
    odbcexception.h
    odbcexception.cpp
    paraminfo.h
    paramitem.h
    paramitem.cpp
    resultinfo.h
    resultinfo.cpp
    tstring.h
	lvstring.h
	lvstring.cpp
    )
 
if (UNIX)
   set(libSrcs ${libSrcs} tstring.h)
else (UNIX)
   set(libSrcs ${libSrcs} tstring.h)
endif (UNIX)

add_library(odbcquery STATIC ${libSrcs})
 
#install(TARGETS util DESTINATION lib)
