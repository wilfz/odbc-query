set(libSrcs 
    connection.h
    datarow.h
    dbitem.h
    fieldinfo.h
    odbcenvironment.h
    odbcexception.h
    paraminfo.h
    paramitem.h
    query.h
    resultinfo.h
    tstring.h
    connection.cpp
    dbitem.cpp
    fieldinfo.cpp
    odbcenvironment.cpp
    odbcexception.cpp
    paramitem.cpp
    query.cpp
    resultinfo.cpp
    )
 
if (UNIX)
   set(libSrcs ${libSrcs} tstring.h)
else (UNIX)
   set(libSrcs ${libSrcs} tstring.h)
endif (UNIX)

add_library(odbcquery STATIC ${libSrcs})
 
#install(TARGETS util DESTINATION lib)
