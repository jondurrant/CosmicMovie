# Add library cpp files

if (NOT DEFINED PIMORONI_DIR)
    set(PIMORONI_DIR "${CMAKE_CURRENT_LIST_DIR}/lib/pimoroni-pico")
endif()

include_directories(${PIMORONI_DIR}/)

include(${PIMORONI_DIR}/common/pimoroni_i2c.cmake)
include(${PIMORONI_DIR}/common/pimoroni_bus.cmake)
include(${PIMORONI_DIR}/libraries/pico_graphics/pico_graphics.cmake)
include(${PIMORONI_DIR}/libraries/cosmic_unicorn/cosmic_unicorn.cmake)

target_compile_definitions(hershey_fonts INTERFACE
	CPPUTEST_MEM_LEAK_DETECTION_DISABLED=1
)