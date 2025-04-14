cmake_minimum_required(VERSION 3.5)

project(Argon VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

file(GLOB DOC_FILES ${CMAKE_SOURCE_DIR}/docs/*)
source_group("Documentation" FILES ${DOC_FILES})
add_custom_target(Documentation SOURCES ${DOC_FILES})

set(INCLUDE_FILES 
    ${CMAKE_SOURCE_DIR}/include/Argon/Argon.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Ast.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Option.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Scanner.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/StringUtil.hpp
    ${CMAKE_SOURCE_DIR}/include/Argon/Traits.hpp
)

set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/Argon.cpp
    ${CMAKE_SOURCE_DIR}/src/Ast.cpp
    ${CMAKE_SOURCE_DIR}/src/Option.cpp
    ${CMAKE_SOURCE_DIR}/src/Scanner.cpp
    ${CMAKE_SOURCE_DIR}/src/StringUtil.cpp
)

add_library(Argon ${SOURCE_FILES})

target_sources(Argon PUBLIC ${INCLUDE_FILES})
target_include_directories(Argon PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

set_target_properties(Argon PROPERTIES POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(tests)

install(TARGETS Argon DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include/Argon)
install (DIRECTORY ${CMAKE_SOURCE_DIR}/docs DESTINATION docs)

IF(MSVC)
    target_compile_options(Argon PRIVATE /W4)
ELSE()
    target_compile_options(Argon PRIVATE -Wall -Wpedantic)
ENDIF()