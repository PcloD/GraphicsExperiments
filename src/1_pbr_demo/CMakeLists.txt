cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PBR_SOURCE ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/pbr_demo.cpp)

add_executable(1_pbr_demo ${PBR_SOURCE})				

target_link_libraries(1_pbr_demo dwSampleFramework)
