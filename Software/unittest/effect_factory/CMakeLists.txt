# Testspezifische Sources
set(MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../components/effects/effect_factory.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/day_night_effect.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/timeline_effect.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/blinking_effect.cpp
    ${CMAKE_SOURCE_DIR}/mocks.cpp
)

add_executable(test_effect_factory test_effect_factory.cpp ${MANAGER_SOURCES})

target_compile_definitions(test_effect_factory PRIVATE UNIT_TEST)

# Testspezifische Includes
target_include_directories(test_effect_factory PRIVATE 
    ${CMAKE_SOURCE_DIR}/mock_esp
    ${CMAKE_SOURCE_DIR}/../components/effects/include
    ${CMAKE_SOURCE_DIR}/../components/led/include
    ${CMAKE_SOURCE_DIR}/../components/datetime/include
)

target_link_libraries(test_effect_factory gtest gtest_main gmock)

add_test(NAME EffectFactoryTest COMMAND test_effect_factory)