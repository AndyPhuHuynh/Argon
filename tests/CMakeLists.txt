set (INCLUDE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/testing.hpp
)

set (SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/error_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/options_test.cpp
)

add_executable(ArgonTests ${SOURCE_FILES})

target_sources(ArgonTests PRIVATE ${INCLUDE_FILES})

target_link_libraries(ArgonTests PRIVATE Argon)

enable_testing()
add_test(NAME ArgonTests COMMAND ArgonTests)