# command line executable qx
include(FindODBC)

set(qxSrcs qx.h qx.cpp)
if (MSVC)
set_source_files_properties(qx.cpp PROPERTIES 
    COMPILE_DEFINITIONS _SCL_SECURE_NO_WARNINGS
	COMPILE_FLAGS /W2)
endif (MSVC)
add_executable(qx ${qxSrcs})

if (${ODBC_FOUND})
    message("ODBC found")
    include_directories(${CMAKE_SOURCE_DIR}/query ${ODBC_INCLUDE_DIRS})
	if (WIN32)
	    target_link_libraries(qx odbcquery ${ODBC_LIBRARIES})
	else()
        target_link_libraries(qx odbcquery ${ODBC_LIBRARIES} pthread)
	endif()
elseif(NOT ${ODBC_FOUND})
    message("ODBC not found")
    include_directories(${CMAKE_SOURCE_DIR}/query)
	if (WIN32)
        target_link_libraries(qx odbcquery)
	else()
        target_link_libraries(qx odbcquery pthread)
	endif()
endif()

install(TARGETS qx DESTINATION bin)
