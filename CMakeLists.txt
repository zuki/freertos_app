cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(demo
        main.c
        ssd1331.c
        bme280.c
        )

target_include_directories(demo PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        /Users/dspace/FreeRTOS/FreeRTOS/Demo/Common/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
        ${PICO_SDK_PATH}/src/rp2_common/hardware_i2c/include)

target_compile_definitions(demo PRIVATE
        PICO_STDIO_STACK_BUFFER_SIZE=64 # use a small printf on stack buffer
)

target_compile_options( demo PUBLIC
        ### Gnu/Clang C Options
        $<$<COMPILE_LANG_AND_ID:C,GNU>:-fdiagnostics-color=always>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-fcolor-diagnostics>

        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wall>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Wextra>
        $<$<COMPILE_LANG_AND_ID:C,Clang,GNU>:-Werror>
        $<$<COMPILE_LANG_AND_ID:C,Clang>:-Weverything>
        )

target_link_libraries(demo pico_stdlib hardware_spi hardware_i2c FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)
pico_add_extra_outputs(demo)
