

set(TOP ${PICO_SDK_PATH}/lib/tinyusb)
message(STATUS "TOP is set to ${TOP}")

add_executable(${NAME}
    main.cpp
    diskImp.c
    tuhMount.c
    #msc_app.c
    #board.c
    
    ${TOP}/lib/fatfs/source/ff.c
    ${TOP}/lib/fatfs/source/ffsystem.c
    ${TOP}/lib/fatfs/source/ffunicode.c
    
)

target_include_directories(${NAME} PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}
     ${TOP}/lib/fatfs/source
     ${TOP}/lib/embedded-cli
     ${TOP}/lib/hw
)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(${NAME} 
    pico_stdlib 
    tinyusb_host
    SPNG
    
    hardware_pio 
	hardware_adc 
	hardware_dma 
	pico_graphics 
	cosmic_unicorn
    )

# enable usb output, disable uart output
pico_enable_stdio_usb(${NAME} 0)
pico_enable_stdio_uart(${NAME} 0)
#target_compile_definitions(${NAME} PRIVATE
#    PICO_DEFAULT_UART_RX_PIN=16
#    PICO_DEFAULT_UART_TX_PIN=17
#)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})