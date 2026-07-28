cmake_minimum_required(VERSION 3.14)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(readelf_toybox_alone VERSION 0.8.12 LANGUAGES C)
endif()

find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT CMAKE_NM OR NOT CMAKE_OBJCOPY OR NOT CMAKE_RANLIB)
    message(FATAL_ERROR
        "The toolchain must provide nm, objcopy, and ranlib")
endif()

file(GLOB READELF_TOYBOX_LIB_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/*.c
)

add_library(readelf_toybox_raw STATIC
    main.c
    toys/other/readelf.c
    ${READELF_TOYBOX_LIB_SOURCES}
)
target_include_directories(readelf_toybox_raw PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(readelf_toybox_raw PRIVATE
    TOYBOX_VERSION="0.8.12"
)
target_compile_options(readelf_toybox_raw PRIVATE
    -fno-lto
    -funsigned-char
    -ffunction-sections
    -fdata-sections
    -Wno-char-subscripts
    -Wno-format-overflow
    -Wno-pointer-sign
    -Wno-string-plus-int
    -Wno-unused-result
)
set_target_properties(readelf_toybox_raw PROPERTIES
    OUTPUT_NAME readelf-toybox-raw
    POSITION_INDEPENDENT_CODE ON
)

set(READELF_TOYBOX_NAMESPACED_ARCHIVE
    ${CMAKE_CURRENT_BINARY_DIR}/libreadelf-toybox-namespaced.a
)
add_custom_command(
    OUTPUT ${READELF_TOYBOX_NAMESPACED_ARCHIVE}
    COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/namespace_static_archive.py
        --nm ${CMAKE_NM}
        --objcopy ${CMAKE_OBJCOPY}
        --ranlib ${CMAKE_RANLIB}
        --prefix yukisu_toybox_
        --input $<TARGET_FILE:readelf_toybox_raw>
        --output ${READELF_TOYBOX_NAMESPACED_ARCHIVE}
    DEPENDS
        readelf_toybox_raw
        ${CMAKE_CURRENT_SOURCE_DIR}/scripts/namespace_static_archive.py
    VERBATIM
    COMMENT "Namespacing Toybox readelf symbols..."
)
add_custom_target(readelf_toybox_namespaced
    DEPENDS ${READELF_TOYBOX_NAMESPACED_ARCHIVE}
)

add_library(readelf_toybox INTERFACE)
add_dependencies(readelf_toybox readelf_toybox_namespaced)
target_link_libraries(readelf_toybox INTERFACE
    ${READELF_TOYBOX_NAMESPACED_ARCHIVE}
    m
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    enable_testing()
    add_executable(readelf standalone_main.c)
    target_link_libraries(readelf PRIVATE readelf_toybox)
    add_test(
        NAME readelf_header
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_readelf.py
            $<TARGET_FILE:readelf>
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/files/elf/ndk-elf-note-full
    )
endif()
