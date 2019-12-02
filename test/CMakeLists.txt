cmake_minimum_required(VERSION 3.2)
project(all_tests)
set(DPDKPATH $ENV{RTE_SDK}/$ENV{RTE_TARGET})

link_directories(${DPDKPATH}/lib)

aux_source_directory(src TEST_FILES)
add_executable(all_tests
    ${TEST_FILES}
)

target_include_directories(all_tests
    PRIVATE lib/googletest/googletest/include
    ../${PROJECT_INCLUDE_DIR}
)

target_compile_options(all_tests
    PUBLIC -Wall -g -O0 -Wno-register
)

target_link_libraries(all_tests
    gtest 
)

## dpdk
target_include_directories(all_tests
    PRIVATE ${DPDKPATH}/include
)

target_compile_options(all_tests
    PRIVATE -m64 -pthread -march=native
)

SET(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")
target_link_libraries(all_tests
    ${DPDK_LIBS} pthread dl rt m pcap numa
)

add_subdirectory(lib/googletest)