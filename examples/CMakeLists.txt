source_group("Examples" FILES basic_input.cpp advanced_usage.cpp)

add_executable(basic_input basic_input.cpp)
target_link_libraries(basic_input PRIVATE Argon)
target_include_directories(basic_input PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(option_chaining option_chaining.cpp)
target_link_libraries(option_chaining PRIVATE Argon)
target_include_directories(option_chaining PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(conversion_function conversion_function.cpp)
target_link_libraries(conversion_function PRIVATE Argon)
target_include_directories(conversion_function PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(custom_error_msg custom_error_msg.cpp)
target_link_libraries(custom_error_msg PRIVATE Argon)
target_include_directories(custom_error_msg PRIVATE ${PROJECT_SOURCE_DIR}/include)

add_executable(multiple_inputs multiple_inputs.cpp)
target_link_libraries(multiple_inputs PRIVATE Argon)
target_include_directories(multiple_inputs PRIVATE ${PROJECT_SOURCE_DIR}/include)
