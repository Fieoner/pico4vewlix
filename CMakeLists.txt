cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(dual_arcade_gamepad C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(dual_arcade_gamepad
    main.c
    usb_descriptors.c
    xinput_driver.c
)

target_include_directories(dual_arcade_gamepad PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(dual_arcade_gamepad PRIVATE pico_stdlib tinyusb_device tinyusb_board)
pico_add_extra_outputs(dual_arcade_gamepad)