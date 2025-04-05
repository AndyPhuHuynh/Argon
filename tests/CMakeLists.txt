add_executable(ArgonTests test_main.cpp)
target_link_libraries(ArgonTests PRIVATE Argon)
enable_testing()
add_test(NAME ArgonTests COMMAND ArgonTests)