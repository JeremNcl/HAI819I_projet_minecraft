# CMake entry point
cmake_minimum_required (VERSION 3.5)
project (VoxelEngine)

find_package(OpenGL REQUIRED)

# Sécurité pour obliger la compilation dans un dossier séparé
if( CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR )
    message( FATAL_ERROR "Veuillez compiler dans un dossier 'build/' !" )
endif()

# Dépendances externes
add_subdirectory (external)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(MSVCMultipleProcessCompile) 

# Dossiers d'inclusion
include_directories(
	external/glfw-3.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
	external/imgui-1.91.0/
	external/imgui-1.91.0/backends/
	src/   # Permet de faire: #include "engine/render/mesh.hpp"
	.
)

set(ALL_LIBS
	${OPENGL_LIBRARY}
	glfw
	GLEW_1130
)

add_definitions(
	-DGLEW_STATIC
	-D_CRT_SECURE_NO_WARNINGS
	-DIMGUI_IMPL_OPENGL_LOADER_GLEW
	-D_GLFW_X11
)

# === MAGIE CMAKE ===
# Récupération automatique de tous les fichiers .cpp et .hpp dans le dossier src/ et ses sous-dossiers
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")

# ImGui sources
set(IMGUI_SOURCES
	external/imgui-1.91.0/imgui.cpp
	external/imgui-1.91.0/imgui_demo.cpp
	external/imgui-1.91.0/imgui_draw.cpp
	external/imgui-1.91.0/imgui_tables.cpp
	external/imgui-1.91.0/imgui_widgets.cpp
	external/imgui-1.91.0/backends/imgui_impl_opengl3.cpp
)

# Création de l'exécutable
add_executable(VoxelEngine ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

target_link_libraries(VoxelEngine ${ALL_LIBS})

# On demande à CMake de placer l'exécutable directement dans le dossier build/ 
set_target_properties(VoxelEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

# Cible pour lancer le programme depuis la racine du projet
add_custom_target(run
	COMMAND cd ${CMAKE_SOURCE_DIR} && ${CMAKE_BINARY_DIR}/VoxelEngine
	DEPENDS VoxelEngine
	COMMENT "Launching VoxelEngine from project root..."
)
