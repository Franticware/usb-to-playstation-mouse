cmake_minimum_required(VERSION 3.13)

add_executable(usb_to_ps1_mouse)

# Example source
target_sources(usb_to_ps1_mouse PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/hid_app.c
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(usb_to_ps1_mouse PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_host
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(usb_to_ps1_mouse PUBLIC pico_stdlib tinyusb_host tinyusb_board pico_multicore)

pico_add_extra_outputs(usb_to_ps1_mouse)
