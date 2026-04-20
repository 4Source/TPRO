# Testspezifische Sources
set(MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../components/effects/effect_factory.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/day_night_effect.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/timeline_effect.cpp
    ${CMAKE_SOURCE_DIR}/../components/effects/blinking_effect.cpp
    ${CMAKE_SOURCE_DIR}/../components/time/datetime.cpp
    ${CMAKE_SOURCE_DIR}/mocks.cpp
)

add_executable(test_effects test_effects.cpp ${MANAGER_SOURCES})

target_compile_definitions(test_effects PRIVATE UNIT_TEST)

# Testspezifische Includes
target_include_directories(test_effects PRIVATE 
    ${CMAKE_SOURCE_DIR}/mock_esp
    ${CMAKE_SOURCE_DIR}/../components/effects/include
    ${CMAKE_SOURCE_DIR}/../components/led/include
    ${CMAKE_SOURCE_DIR}/../components/time/include
)

target_link_libraries(test_effects gtest gtest_main gmock)

add_test(NAME SerializationTests COMMAND test_effects)