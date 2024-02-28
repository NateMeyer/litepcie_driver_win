##
# Simple Test application to excercise the litepcie driver and library
## 
add_executable(litepcie_test litepcie_test.cpp)

target_link_libraries(litepcie_test litepcie)
target_link_libraries(litepcie_test setupapi)
