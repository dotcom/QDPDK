cmake_minimum_required(VERSION 3.2)
set(PROJ_NAME examle)
project(${PROJ_NAME})
set(DPDKPATH $ENV{RTE_SDK}/$ENV{RTE_TARGET})

link_directories(${DPDKPATH}/lib)

aux_source_directory(src TEST_FILES)
add_executable(${PROJ_NAME}
    ${TEST_FILES}
)

target_include_directories(${PROJ_NAME}
    PRIVATE lib/googletest/googletest/include
    ../${PROJECT_INCLUDE_DIR}
)

target_compile_options(${PROJ_NAME}
    PRIVATE -Wall -g -O0 -Wno-register
)

## dpdk
target_include_directories(${PROJ_NAME}
    PRIVATE ${DPDKPATH}/include
)

target_compile_options(${PROJ_NAME}
    PRIVATE -m64 -pthread -march=native
)

SET(DPDK_LIBS "-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive")
target_link_libraries(${PROJ_NAME}
    ${DPDK_LIBS} pthread dl rt m pcap numa
)