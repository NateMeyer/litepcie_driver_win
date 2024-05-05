##
# LitePCIe Driver and Library Project
##
cmake_minimum_required(VERSION 3.20)

enable_language(C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(litepcie)

set(CMAKE_CONFIGURATION_TYPES Debug Release)


add_compile_options($<$<CONFIG:RELEASE>:-O2>)


add_definitions(-DUNICODE -D_UNICODE)


set(litepciedrv_PUBLIC_HEADERS 
    ${CMAKE_SOURCE_DIR}/litepciedrv/public_h/csr.h
    ${CMAKE_SOURCE_DIR}/litepciedrv/public_h/soc.h
    ${CMAKE_SOURCE_DIR}/litepciedrv/public_h/litepcie_dmadrv.h
    ${CMAKE_SOURCE_DIR}/litepciedrv/public_h/litepcie.h
    )

add_subdirectory(liblitepcie)
add_subdirectory(litepcie_test)
