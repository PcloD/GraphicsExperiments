cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CDLOD_SOURCE ${PROJECT_SOURCE_DIR}/src/2_cdlod/cdlod.cpp)

add_executable(2_cdlod ${CDLOD_SOURCE})				

target_link_libraries(2_cdlod dwSampleFramework)
