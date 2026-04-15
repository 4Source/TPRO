# Testspezifische Sources
set(MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../components/light_effect_manager/light_effect_manager.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/blinking_effect.cpp 
    ${CMAKE_SOURCE_DIR}/../components/config_manager/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/../components/time/datetime.cpp
)

add_executable(test_light_effect_manager test_led_effect_manager.cpp ${MANAGER_SOURCES})

# Testspezifische Includes
target_include_directories(test_light_effect_manager PRIVATE 
    ${CMAKE_SOURCE_DIR}/mock_esp
    ${CMAKE_SOURCE_DIR}/../components/light_effect_manager/include
    ${CMAKE_SOURCE_DIR}/../components/config_manager/include
    ${CMAKE_SOURCE_DIR}/../components/effects/include
    ${CMAKE_SOURCE_DIR}/../components/led/include
    ${CMAKE_SOURCE_DIR}/../components/task_handles/include
    ${CMAKE_SOURCE_DIR}/../components/time/include
)

target_link_libraries(test_light_effect_manager gtest gtest_main)

add_test(NAME LightManagerTest COMMAND test_light_effect_manager)