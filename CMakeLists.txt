cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

get_directory_property(hasParent PARENT_DIRECTORY)

if (NOT hasParent)
    include(lib/pico-sdk/pico_sdk_init.cmake)
endif()

project(pico-web-client)

pico_sdk_init()

add_library(pico_web_client
    src/tcp_client.cpp
    src/tcp_tls_client.cpp
    src/circular_buffer.cpp
    src/http_client.cpp
    src/websocket.cpp
    src/eio_client.cpp
    src/sio_client.cpp
    src/LUrlParser.cpp
)

target_include_directories(pico_web_client PUBLIC include lib/json/single_include)
target_link_libraries(pico_web_client PRIVATE
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    pico_multicore
)
target_compile_options(pico_web_client PRIVATE "-Wno-psabi")