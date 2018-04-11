cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(PSSM_SOURCE ${PROJECT_SOURCE_DIR}/src/5_pssm/pssm.cpp)

add_executable(5_pssm ${PSSM_SOURCE})				

target_link_libraries(5_pssm dwSampleFramework)
