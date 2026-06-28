add_library(renderer
    SWRenderer.cpp
    rendererUtil.cpp
)

target_compile_options(renderer PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(renderer PRIVATE
    ${SPSX_LDFLAGS}
)

target_link_libraries(renderer PRIVATE
    util
    core
)

target_include_directories(renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
