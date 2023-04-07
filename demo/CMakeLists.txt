# demo executable
include(FindODBC)

set(demoSrcs main.cpp)
add_executable(demo ${demoSrcs})

if (${ODBC_FOUND})
    message("ODBC found")
    include_directories(${CMAKE_SOURCE_DIR}/query ${ODBC_INCLUDE_DIRS})
    target_link_libraries(demo odbcquery ${ODBC_LIBRARIES})
elseif(NOT ${ODBC_FOUND})
    message("ODBC not found")
    include_directories(${CMAKE_SOURCE_DIR}/query)
    target_link_libraries(demo odbcquery)
endif()

#install(TARGETS demo DESTINATION bin)
