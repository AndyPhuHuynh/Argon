cmake_minimum_required(VERSION 3.5)

project(Argon VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

file(GLOB DOC_FILES ${CMAKE_SOURCE_DIR}/docs/*)
source_group("Documentation" FILES ${DOC_FILES})
add_custom_target(Documentation SOURCES ${DOC_FILES})

set(INCLUDE_FILES
    ${CMAKE_SOURCE_DIR}/include/Argon/*.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Config/*.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Options/*.hpp
)

add_library(Argon INTERFACE)

target_compile_features(Argon INTERFACE cxx_std_23)

target_include_directories(Argon INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(Argon PROPERTIES POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)

install(FILES ${INCLUDE_FILES} DESTINATION include/Argon)
install (DIRECTORY ${CMAKE_SOURCE_DIR}/docs DESTINATION docs)

IF(MSVC)
    target_compile_options(Argon INTERFACE /W4 /utf-8)
ELSE()
    target_compile_options(Argon INTERFACE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wswitch-enum)
ENDIF()