cmake_minimum_required(VERSION 3.20)
project(OTCv8ProxyClient VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR})

# Find required packages via vcpkg
find_package(Boost REQUIRED COMPONENTS system)
find_package(ZLIB REQUIRED)

# GMP is typically found via pkg-config or manual find
find_path(GMP_INCLUDE_DIR gmp.h)
find_library(GMP_LIBRARY NAMES gmp libgmp)

if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY)
    message(FATAL_ERROR "GMP library not found")
endif()

# Source files
set(SOURCES
        src/main.cpp
        src/client.cpp
        src/server.cpp
        src/rsa.cpp
)

set(HEADERS
        src/client.h
        src/server.h
        src/rsa.h
        src/crypt.h
        src/resource.h
)

# Windows resource file
if(WIN32)
    set(RESOURCES src/resource.rc)
endif()

# Debug build (console application)
add_executable(proxy_debug ${SOURCES} ${HEADERS})
target_compile_definitions(proxy_debug PRIVATE DEBUG)
target_include_directories(proxy_debug PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${GMP_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(proxy_debug PRIVATE 
    Boost::system
    ${GMP_LIBRARY}
    ZLIB::ZLIB
)
if(WIN32)
    target_link_libraries(proxy_debug PRIVATE ws2_32 wsock32 shlwapi)
endif()

# Release build (Windows GUI application with icon/resources)
add_executable(launcher WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})
target_include_directories(launcher PRIVATE 
    ${Boost_INCLUDE_DIRS}
    ${GMP_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)
target_link_libraries(launcher PRIVATE 
    Boost::system
    ${GMP_LIBRARY}
    ZLIB::ZLIB
)
if(WIN32)
    target_link_libraries(launcher PRIVATE ws2_32 wsock32 shlwapi)
    if(MINGW)
        target_link_options(launcher PRIVATE -static)
    endif()
endif()

# Static runtime for release builds (optional, for standalone exe)
if(MSVC)
    target_link_options(launcher PRIVATE "/SAFESEH:NO" "/DEBUG:NONE")
    target_link_options(proxy_debug PRIVATE "/SAFESEH:NO" "/DEBUG:NONE")
    set_property(TARGET launcher PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set_property(TARGET proxy_debug PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
