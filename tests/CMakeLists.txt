set (INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Testing.hpp
)

set (SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ScannerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
)

add_executable(ArgonTests ${SOURCE_FILES})

target_sources(ArgonTests PRIVATE ${INCLUDE_FILES})

target_link_libraries(ArgonTests PRIVATE Argon)

enable_testing()
add_test(NAME ArgonTests COMMAND ArgonTests)