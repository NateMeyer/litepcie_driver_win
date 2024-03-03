set(litepcie_SOURCES
    src/litepcie_dma.c
    src/litepcie_flash.c
    src/litepcie_helpers.c
    )

set(litepcie_HEADERS
    include/liblitepcie.h
    include/litepcie_dma.h
    include/litepcie_flash.h
    include/litepcie_helpers.h
    )

add_library(litepcie STATIC ${litepcie_SOURCES} ${litepcie_HEADERS})

#target_include_directories(litepcie PUBLIC include)
target_include_directories(litepcie PUBLIC include ${CMAKE_SOURCE_DIR}/litepciedrv/public_h)
