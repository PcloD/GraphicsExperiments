cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(GRAPHICS_DEMO_SOURCE ${PROJECT_SOURCE_DIR}/src/7_graphics_demo/graphics_demo.cpp
                         ${PROJECT_SOURCE_DIR}/src/7_graphics_demo/render_graph.h
                         ${PROJECT_SOURCE_DIR}/src/7_graphics_demo/render_node.h
                         ${PROJECT_SOURCE_DIR}/src/7_graphics_demo/render_graph.cpp
                         ${PROJECT_SOURCE_DIR}/src/7_graphics_demo/render_node.cpp)

add_executable(7_graphics_demo ${PBR_SOURCE})				

target_link_libraries(7_graphics_demo dwSampleFramework)