# command line executable qx
include(FindODBC)

set(qxSrcs CLI11.hpp SimpleIni.h qx.cpp)
add_executable(qx ${qxSrcs})

if (${ODBC_FOUND})
    message("ODBC found")
    include_directories(${CMAKE_SOURCE_DIR}/query ${ODBC_INCLUDE_DIRS})
    target_link_libraries(qx odbcquery ${ODBC_LIBRARIES})
elseif(NOT ${ODBC_FOUND})
    message("ODBC not found")
    include_directories(${CMAKE_SOURCE_DIR}/query)
    target_link_libraries(qx odbcquery)
endif()

#install(TARGETS qx DESTINATION bin)
