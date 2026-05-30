add_library(gdb
    Network.cpp
)

target_compile_options(gdb PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(gdb PRIVATE
    ${SPSX_LDFLAGS}
)

target_include_directories(gdb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
