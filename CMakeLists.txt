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
    src/allocator.cpp
    src/iequals.cpp
    src/tcp_client.cpp
    src/tcp_tls_client.cpp
    src/udp_client.cpp
    src/ntp_client.cpp
    src/circular_buffer.cpp
    src/circular_iterator.cpp
    src/circular_view.cpp
    src/http_request.cpp
    src/http_response.cpp
    src/http_client.cpp
    src/mqtt_client.cpp
    src/mqtt/properties.cpp
    src/mqtt/varint.cpp
    src/mqtt/packet/base.cpp
    src/mqtt/packet/connect.cpp
    src/mqtt/packet/connack.cpp
    src/mqtt/packet/publish.cpp
    src/mqtt/packet/puback.cpp
    src/mqtt/packet/pubrec.cpp
    src/mqtt/packet/pubrel.cpp
    src/mqtt/packet/pubcomp.cpp
    src/mqtt/packet/subscribe.cpp
    src/mqtt/packet/suback.cpp
    src/mqtt/packet/unsubscribe.cpp
    src/mqtt/packet/unsuback.cpp
    src/mqtt/packet/pingreq.cpp
    src/mqtt/packet/pingresp.cpp
    src/mqtt/packet/disconnect.cpp
    src/mqtt/packet/auth.cpp
    src/websocket.cpp
    src/eio_client.cpp
    src/sio_client.cpp
    src/sio_packet.cpp
    src/sio_socket.cpp
    src/LUrlParser.cpp
    src/logger.cpp
    src/wifi_utils.cpp
)

target_include_directories(pico_web_client PUBLIC include lib/json/single_include)
target_link_libraries(pico_web_client PRIVATE
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    pico_multicore
    hardware_rtc
)
target_compile_options(pico_web_client PRIVATE "-Wno-psabi")
target_compile_definitions(pico_web_client PRIVATE 
    "LOG_LEVEL=$ENV{LOG_LEVEL}"
)

add_executable(pico_download_test
    test/download_test.cpp
)

target_include_directories(pico_download_test PUBLIC include)
target_link_libraries(pico_download_test PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    pico_multicore
    pico_stdlib
    pico_web_client
)
target_compile_options(pico_download_test PRIVATE "-Wno-psabi")
target_compile_definitions(pico_download_test PRIVATE 
    "WIFI_SSID=\"$ENV{WIFI_SSID}\""
    "WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\""
    "LOG_LEVEL=$ENV{LOG_LEVEL}"
)
target_link_options(pico_download_test PRIVATE "-Wl,--print-memory-usage")

pico_enable_stdio_usb(pico_download_test 1)
pico_enable_stdio_uart(pico_download_test 0)
pico_add_extra_outputs(pico_download_test)

add_executable(pico_mqtt_test
    test/mqtt_test.cpp
)

target_include_directories(pico_mqtt_test PUBLIC include)
target_link_libraries(pico_mqtt_test PRIVATE
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_mbedtls
    pico_mbedtls
    pico_multicore
    pico_stdlib
    pico_web_client
)
target_compile_options(pico_mqtt_test PRIVATE "-Wno-psabi")
target_compile_definitions(pico_mqtt_test PRIVATE 
    "WIFI_SSID=\"$ENV{WIFI_SSID}\""
    "WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\""
    "MQTT_USERNAME=\"$ENV{MQTT_USERNAME}\""
    "MQTT_PASSWORD=\"$ENV{MQTT_PASSWORD}\""
    "LOG_LEVEL=$ENV{LOG_LEVEL}"
    "PICO_DEFAULT_UART_TX_PIN=12"
    "PICO_DEFAULT_UART_RX_PIN=13"
)
target_link_options(pico_mqtt_test PRIVATE "-Wl,--print-memory-usage")

pico_enable_stdio_usb(pico_mqtt_test 0)
pico_enable_stdio_uart(pico_mqtt_test 1)
pico_add_extra_outputs(pico_mqtt_test)