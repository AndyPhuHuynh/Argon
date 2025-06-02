include (FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable(Catch2)

set (SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/attributes_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/error_tests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/scanner_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/options_test.cpp
)

add_executable(ArgonTests ${SOURCE_FILES})

target_link_libraries(ArgonTests PRIVATE Argon Catch2::Catch2WithMain)

enable_testing()
add_test(NAME ArgonTests COMMAND ArgonTests)

include(Catch)
catch_discover_tests(ArgonTests)