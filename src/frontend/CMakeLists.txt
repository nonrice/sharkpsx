add_executable(${PROJECT_NAME}
    main.cpp
    App.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE
    ${SPSX_CXXFLAGS}
)

target_link_options(${PROJECT_NAME} PRIVATE
    ${SPSX_LDFLAGS}
)

find_package(SDL3 REQUIRED CONFIG)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    core
    renderer
    util
    SDL3::SDL3
)
