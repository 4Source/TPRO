# Testspezifische Sources
set(MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../components/time/datetime.cpp
)

add_executable(test_datetime test_datetime.cpp ${MANAGER_SOURCES})

# Testspezifische Includes
target_include_directories(test_datetime PRIVATE 
    ${CMAKE_SOURCE_DIR}/../components/time/include

)

target_link_libraries(test_datetime gtest gtest_main)

add_test(NAME DateTimeTest COMMAND test_datetime)