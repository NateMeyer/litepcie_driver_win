###
# Add CMake commands to build the litepcie Windows driver
###

#[[
set(litepciedrv_BIN_FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLAFORM_NAME}/${CMAKE_BUILD_TYPE}/litepciedrv/litepciedrv.cat"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLAFORM_NAME}/${CMAKE_BUILD_TYPE}/litepciedrv/litepciedrv.inf"
    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLAFORM_NAME}/${CMAKE_BUILD_TYPE}/litepciedrv/litepciedrv.sys"
)

add_custom_target(litepciedrv
    DEPENDS
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLAFORM_NAME}/${CMAKE_BUILD_TYPE}/litepciedrv/litepciedrv.sys"
)

add_custom_command(
    OUTPUT
        "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_VS_PLAFORM_NAME}/${CMAKE_BUILD_TYPE}/litepciedrv/litepciedrv.sys"
    COMMAND
        msbuild "litepciedrv.vcxproj" /p:configuration="${CMAKE_BUILD_TYPE}" /p:platform="${CMAKE_VS_PLATFORM_NAME}"
    BYPRODUCTS ${litepciedrv_BIN_FILES}
)
#]]

set(litepciedrv_SOURCES
    src/Driver.c
    src/Device.c
    src/Queue.c
    )

set(litepciedrv_HEADERS
    include/Device.h
    include/Driver.h
    include/Queue.h
    include/Trace.h
    )

set(litepciedrv_PUBLIC_HEADERS
    public_h/litepcie_dmadrv.h
    public_h/litepcie.h
    public_h/csr.h
    public_h/soc.h
    )

#add_library(litepciedrv STATIC ${litepciedrv_SOURCES} ${litepciedrv_HEADERS} ${litepciedrv_PUBLIC_HEADERS})
wdk_add_driver(litepciedrv KMDF 1.33 WINVER 10 ${litepciedrv_SOURCES} ${litepciedrv_HEADERS} ${litepciedrv_PUBLIC_HEADERS})

#<PlatformToolset>WindowsKernelModeDriver10.0</PlatformToolset>
set_target_properties(litepciedrv 
                    PROPERTIES VS_PLATFORM_TOOLSET "WindowsKernelModeDriver10.0"
                               VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10"
 #                              VS_CONFIGURATION_TYPE "Driver"
 #                              VS_DRIVER_TYPE "KMDF"
 #                              VS_DRIVER_TARGET_PLATFORM "Universal"
 )

#target_include_directories(litepcie PUBLIC include)
target_include_directories(litepciedrv PUBLIC public_h PRIVATE include)

#target_link_libraries(litepciedrv WdfDriverEntry WdfLdr)