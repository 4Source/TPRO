# Testspezifische Sources
set(MANAGER_SOURCES
    ${CMAKE_SOURCE_DIR}/../components/webserver/websocket_server.cpp
    ${CMAKE_SOURCE_DIR}/mocks.cpp
)

add_executable(test_websocket_server test_websocket_server.cpp ${MANAGER_SOURCES})

target_compile_definitions(test_websocket_server PRIVATE UNIT_TEST)

# Testspezifische Includes
target_include_directories(test_websocket_server PRIVATE 
    ${CMAKE_SOURCE_DIR}/mock_esp
    ${CMAKE_SOURCE_DIR}/../components/webserver/include
    ${CMAKE_SOURCE_DIR}/../components/led/include
    ${CMAKE_SOURCE_DIR}/../components/task_handles/include
)

target_link_libraries(test_websocket_server gtest gtest_main)

add_test(NAME WebSocketTest COMMAND test_websocket_server)