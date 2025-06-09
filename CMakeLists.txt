cmake_minimum_required(VERSION 3.16)
project(ClueEngine)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Detect OS
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(PLATFORM_WINDOWS TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM_MACOS TRUE)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(PLATFORM_LINUX TRUE)
endif()

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# Set multi-config directories (for Visual Studio)
if (CMAKE_CONFIGURATION_TYPES) # Multi-config generators (Visual Studio)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin)
    endforeach()
else() # Single-config generators (Makefile, Ninja)  
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
endif()

# Find OpenGL
find_package(OpenGL REQUIRED)

# Find OpenAL (AUDIO SYSTEM)
find_package(OpenAL REQUIRED)
if(OpenAL_FOUND)
    message(STATUS "OpenAL found: ${OPENAL_LIBRARY}")
else()
    message(WARNING "OpenAL not found - building without audio support")
endif()

# Find required system dependencies first
find_package(ZLIB REQUIRED)

# Fetch External Dependencies
include(FetchContent)

# Fetch and build cJSON
FetchContent_Declare(
  cjson
  GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git
  GIT_TAG        master
)
FetchContent_GetProperties(cjson)
if(NOT cjson_POPULATED)
    FetchContent_Populate(cjson)
    add_subdirectory(${cjson_SOURCE_DIR} ${cjson_BINARY_DIR})
endif()
include_directories(${cjson_SOURCE_DIR})

# Fetch and build tinyfiledialogs
FetchContent_Declare(
  tinyfiledialogs
  GIT_REPOSITORY https://github.com/native-toolkit/tinyfiledialogs.git
  GIT_TAG        master
)
FetchContent_GetProperties(tinyfiledialogs)
if(NOT tinyfiledialogs_POPULATED)
    FetchContent_Populate(tinyfiledialogs)
    add_library(tinyfiledialogs STATIC ${tinyfiledialogs_SOURCE_DIR}/tinyfiledialogs.c)
endif()
include_directories(${tinyfiledialogs_SOURCE_DIR})

# Assimp (3D Model Loader) - FIXED VERSION
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
# Only enable the importers we need
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_PLY_IMPORTER ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v5.4.3
)
FetchContent_MakeAvailable(assimp)

# SOIL2 (Image Loader)
FetchContent_Declare(
  soil2
  GIT_REPOSITORY https://github.com/SpartanJ/soil2.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(soil2)
include_directories(${soil2_SOURCE_DIR}/src)

# Download stb_vorbis for audio support
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/stb_vorbis.c)
    message(STATUS "Downloading stb_vorbis...")
    file(DOWNLOAD 
        "https://raw.githubusercontent.com/nothings/stb/master/stb_vorbis.c"
        "${CMAKE_SOURCE_DIR}/external/stb_vorbis.c"
        SHOW_PROGRESS
    )
endif()

# Find GLFW (Windows / Linux / MacOS)
if (PLATFORM_WINDOWS)
    # Try to find prebuilt GLFW first, then build from source
    find_library(GLFW_LIBRARY NAMES glfw3 glfw PATHS "${CMAKE_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
    if(NOT GLFW_LIBRARY)
        message(STATUS "GLFW not found in lib directory, building from source...")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.3.8)
        FetchContent_MakeAvailable(glfw)
        set(GLFW_LIBRARIES glfw)
    else()
        message(STATUS "Using prebuilt GLFW: ${GLFW_LIBRARY}")
        set(GLFW_LIBRARIES ${GLFW_LIBRARY})
    endif()
elseif (PLATFORM_LINUX OR PLATFORM_MACOS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW QUIET glfw3)
    if (NOT GLFW_FOUND)
        find_package(glfw3 CONFIG QUIET)
    endif()
    if (NOT GLFW_FOUND)
        find_path(GLFW_INCLUDE_DIR NAMES GLFW/glfw3.h PATHS /usr/include /usr/local/include)
        find_library(GLFW_LIBRARY NAMES glfw glfw3 PATHS /usr/lib /usr/local/lib)
        if (GLFW_INCLUDE_DIR AND GLFW_LIBRARY)
            set(GLFW_FOUND TRUE)
            set(GLFW_LIBRARIES ${GLFW_LIBRARY})
            include_directories(${GLFW_INCLUDE_DIR})
        endif()
    endif()
    if (NOT GLFW_FOUND)
        message(FATAL_ERROR "GLFW3 not found. Install it with your package manager.")
    endif()
endif()

# Find GLEW (Separate for Windows & Linux/macOS)
if (PLATFORM_WINDOWS)
    # Try to find prebuilt GLEW first, then build from source
    find_library(GLEW_LIBRARY NAMES glew32s glew32 PATHS "${CMAKE_SOURCE_DIR}/lib" NO_DEFAULT_PATH)
    if(NOT GLEW_LIBRARY)
        message(STATUS "GLEW not found in lib directory, building from source...")
        FetchContent_Declare(glew GIT_REPOSITORY https://github.com/nigels-com/glew.git GIT_TAG glew-2.2.0 SOURCE_SUBDIR build/cmake)
        set(glew-cmake_BUILD_SHARED OFF CACHE BOOL "" FORCE)
        set(glew-cmake_BUILD_STATIC ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glew)
        set(GLEW_LIBRARIES libglew_static)
    else()
        message(STATUS "Using prebuilt GLEW: ${GLEW_LIBRARY}")
        set(GLEW_LIBRARIES ${GLEW_LIBRARY})
    endif()
    include_directories(${CMAKE_SOURCE_DIR}/include)
else()
    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIRS})
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external
)

# Source files - INCLUDING AUDIO
file(GLOB_RECURSE SOURCES src/*.c src/*.cpp)
file(GLOB_RECURSE HEADERS include/*.h include/*.hpp)

# Add stb_vorbis implementation
set(SOURCES ${SOURCES} ${CMAKE_SOURCE_DIR}/external/stb_vorbis.c)

# Ensure add_executable is declared BEFORE linking libraries
add_executable(ClueEngine ${SOURCES} ${HEADERS})

# Add preprocessor definition for audio support
if(OpenAL_FOUND)
    target_compile_definitions(ClueEngine PRIVATE AUDIO_ENABLED)
endif()

# Properly link libraries for **each platform**
if (PLATFORM_WINDOWS)
    target_link_libraries(ClueEngine
        OpenGL::GL
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
        assimp
        soil2
        cjson
        tinyfiledialogs
        ZLIB::ZLIB
    )
    # Add OpenAL for Windows
    if(OpenAL_FOUND)
        target_link_libraries(ClueEngine ${OPENAL_LIBRARY})
    endif()
    
elseif (PLATFORM_MACOS)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREVIDEO_FRAMEWORK CoreVideo)      
    target_link_libraries(ClueEngine
        OpenGL::GL
        glfw
        GLEW
        assimp
        soil2
        cjson
        tinyfiledialogs
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ZLIB::ZLIB
    )
    # Add OpenAL for macOS
    if(OpenAL_FOUND)
        target_link_libraries(ClueEngine ${OPENAL_LIBRARY})
    endif()
    
elseif (PLATFORM_LINUX)
    target_link_libraries(ClueEngine
        OpenGL::GL
        ${GLFW_LIBRARIES}
        ${GLEW_LIBRARIES}
        assimp
        soil2
        cjson
        tinyfiledialogs
        ZLIB::ZLIB
        m   
        pthread
        dl
    )
    # Add OpenAL for Linux
    if(OpenAL_FOUND)
        target_link_libraries(ClueEngine ${OPENAL_LIBRARY})
    endif()
endif()

# Copy DLLs (Windows only)
if (PLATFORM_WINDOWS)
    add_custom_command(TARGET ClueEngine POST_BUILD  
        COMMAND ${CMAKE_COMMAND} -E copy_directory   
        ${CMAKE_SOURCE_DIR}/external ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# Copy shaders and resources
add_custom_command(TARGET ClueEngine POST_BUILD      
    COMMAND ${CMAKE_COMMAND} -E copy_directory       
    ${CMAKE_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
)

add_custom_command(TARGET ClueEngine POST_BUILD      
    COMMAND ${CMAKE_COMMAND} -E copy_directory       
    ${CMAKE_SOURCE_DIR}/resources ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources
)

# Create audio directory if it doesn't exist
add_custom_command(TARGET ClueEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/audio
)

# Set working directory in Visual Studio
if (PLATFORM_WINDOWS)
    set_target_properties(ClueEngine PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")  
endif()

# Compiler settings
if (MSVC)
    target_compile_options(ClueEngine PRIVATE /W4)   
else()
    target_compile_options(ClueEngine PRIVATE -Wall -Wextra -Wno-error)
endif()

# Print configuration summary
message(STATUS "=== ClueEngine Build Configuration ===")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "OpenGL: Found")
message(STATUS "OpenAL: ${OpenAL_FOUND}")
if(OpenAL_FOUND)
    message(STATUS "  Library: ${OPENAL_LIBRARY}")
endif()
message(STATUS "GLFW: ${GLFW_FOUND}")
message(STATUS "ZLIB: ${ZLIB_FOUND}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "=====================================")