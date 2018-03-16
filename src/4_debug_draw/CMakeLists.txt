cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(DEBUG_DRAW_SOURCE ${PROJECT_SOURCE_DIR}/src/4_debug_draw/debug_draw.cpp)

add_executable(4_debug_draw ${DEBUG_DRAW_SOURCE})				

target_link_libraries(4_debug_draw dwSampleFramework)
