add_library(util
    SigCapture.cpp
)

target_compile_options(util PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(util PRIVATE
    ${SPSX_LDFLAGS}
)

target_include_directories(util PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
