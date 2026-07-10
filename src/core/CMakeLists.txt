add_library(core
    CPU.cpp
    GPU.cpp
    GTE.cpp
    MemBlockDevice.cpp
    Bus.cpp
    System.cpp
    Debugger.cpp
    BasicDebug.cpp
    PSXServer.cpp
    SIO.cpp
    IntCtl.cpp
    Controller.cpp
)

target_compile_options(core PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(core PRIVATE
    ${SPSX_LDFLAGS}
)

target_link_libraries(core PRIVATE
    util
    gdb
)

target_include_directories(core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
