set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SPSX_CXXFLAGS
    -Wall
    -fsanitize=address,undefined
    -g
    -fno-omit-frame-pointer
)

if (NOT WIN32)
    list(APPEND SPSX_CXXFLAGS
        -Wextra
        -Wpedantic
        -Wconversion
    )
endif()

set(SPSX_LDFLAGS
    -fsanitize=address,undefined
)

add_subdirectory(util)
add_subdirectory(gdb)
add_subdirectory(core)
