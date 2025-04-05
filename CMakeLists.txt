cmake_minimum_required(VERSION 3.5)

project(Argon VERSION 0.0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

set(INCLUDE_FILES 
    ${CMAKE_SOURCE_DIR}/include/Argon/Argon.hpp
)

set(SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/Argon.cpp
)

add_library(Argon ${SOURCE_FILES})

target_sources(Argon PUBLIC ${INCLUDE_FILES})
target_include_directories(Argon PUBLIC
    ${CMAKE_SOURCE_DIR}/include/Argon
)


set_target_properties(Argon PROPERTIES POSITION_INDEPENDENT_CODE ON)

enable_testing()
add_subdirectory(tests)

install(TARGETS Argon DESTINATION lib)
install(FILES ${INCLUDE_FILES} DESTINATION include/Argon)