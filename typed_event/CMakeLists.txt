# Keep a sub_folder cmake file simple and clean. Just add what is needed on top of the root CMakeLitsts.txt
set(MY_EXECUTABLE typed_event)
add_executable(${MY_EXECUTABLE} main.cpp)

# Let's move exectuables to a single place to make finding them easier
set_target_properties(${MY_EXECUTABLE} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
