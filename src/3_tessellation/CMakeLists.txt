cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(TESSELLATION_SOURCE ${PROJECT_SOURCE_DIR}/src/3_tessellation/tessellation.cpp)

add_executable(3_tessellation ${TESSELLATION_SOURCE})				

target_link_libraries(3_tessellation dwSampleFramework)
