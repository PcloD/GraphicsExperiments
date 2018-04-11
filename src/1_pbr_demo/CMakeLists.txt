cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PBR_SOURCE ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/pbr_demo.cpp
               ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/project.h
               ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/render_graph.h
               ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/render_node.h
               ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/render_graph.cpp
               ${PROJECT_SOURCE_DIR}/src/1_pbr_demo/render_node.cpp)

add_executable(1_pbr_demo ${PBR_SOURCE})				

target_link_libraries(1_pbr_demo dwSampleFramework)
target_link_libraries(1_pbr_demo nfd)