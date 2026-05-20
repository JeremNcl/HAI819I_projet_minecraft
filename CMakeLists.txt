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
	external/glfw-3.1.2/include/
	external/glm-0.9.7.1/
	external/glew-1.13.0/include/
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
)

# === MAGIE CMAKE ===
# Récupération automatique de tous les fichiers .cpp et .hpp dans le dossier src/ et ses sous-dossiers
file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")

# Création de l'exécutable
add_executable(VoxelEngine ${SOURCES} ${HEADERS})

target_link_libraries(VoxelEngine ${ALL_LIBS})

# On demande à CMake de placer l'exécutable directement dans le dossier build/ 
set_target_properties(VoxelEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
