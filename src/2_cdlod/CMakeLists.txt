cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CDLOD_SOURCE ${PROJECT_SOURCE_DIR}/src/2_cdlod/cdlod.cpp
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/heightmap.cpp
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/heightmap.h
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/node.h
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/node.cpp
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/terrain.cpp
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/terrain.h
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/terrain_patch.h
                 ${PROJECT_SOURCE_DIR}/src/2_cdlod/terrain_patch.cpp)

add_executable(2_cdlod ${CDLOD_SOURCE})				

target_link_libraries(2_cdlod dwSampleFramework)
