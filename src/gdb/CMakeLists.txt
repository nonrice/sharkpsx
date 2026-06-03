add_library(gdb
    Net.cpp
    Server.cpp
    GDBServer.cpp
)

target_compile_options(gdb PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(gdb PRIVATE
    ${SPSX_LDFLAGS}
)

target_link_libraries(gdb PRIVATE
    util
)

target_include_directories(gdb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
