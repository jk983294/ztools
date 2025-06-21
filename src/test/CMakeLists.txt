file(GLOB tests_src "*.cpp")

enable_testing()

add_executable(test ${tests_src})
target_link_libraries(test zerg pthread rt)
