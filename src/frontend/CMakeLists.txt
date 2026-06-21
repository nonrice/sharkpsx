add_library(frontend
    App.cpp
)

target_compile_options(sharkpsx PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(sharkpsx PRIVATE
    ${SPSX_LDFLAGS}
)

find_package(SDL3 REQUIRED CONFIG)

target_link_libraries(frontend PRIVATE
    util
    SDL3::SDL3
)
