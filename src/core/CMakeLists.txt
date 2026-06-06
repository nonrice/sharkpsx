add_executable(sharkpsx
    main.cpp
    CPU.cpp
    GTE.cpp
    MemBlockDevice.cpp
    Bus.cpp
    System.cpp
    Debugger.cpp
    BasicDebug.cpp
    PSXServer.cpp
)

target_compile_options(sharkpsx PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(sharkpsx PRIVATE
    ${SPSX_LDFLAGS}
)

target_link_libraries(sharkpsx PRIVATE
    util
    gdb
)
