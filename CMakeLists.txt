cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("GraphicsExperiments")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(DWSFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/include")
set(GFX_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/Terminus-GFX/include")
set(MATH_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/Terminus-Math/include")
set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/glfw/include")
set(GLM_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/glm/glm")
set(STB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/stb")
set(IMGUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/imgui")
set(JSON_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/dwSampleFramework/external/json/single_include/nlohmann")
set(NFD_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/nfd/include")

include_directories("${DWSFW_INCLUDE_DIRS}"
                    "${GFX_INCLUDE_DIRS}"
                    "${MATH_INCLUDE_DIRS}"
                    "${GLM_INCLUDE_DIRS}"
                    "${STB_INCLUDE_DIRS}"
                    "${IMGUI_INCLUDE_DIRS}"
                    "${JSON_INCLUDE_DIRS}"
                    "${GLFW_INCLUDE_DIRS}")

add_subdirectory(external/dwSampleFramework)
add_subdirectory(external/nfd)

# Experiments
add_subdirectory(src/1_pbr_demo)
add_subdirectory(src/2_cdlod)
add_subdirectory(src/3_tessellation)
add_subdirectory(src/4_debug_draw)
add_subdirectory(src/5_pssm)