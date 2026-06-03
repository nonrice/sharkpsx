set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SPSX_CXXFLAGS
    -fsanitize=address,undefined
    -g
    -fno-omit-frame-pointer
)

if (NOT WIN32) # Darwin/Linux
    list(APPEND SPSX_CXXFLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
    )
else()
    list(APPEND SPSX_CXXFLAGS
        -Zc:preprocessor # use standard compliant preprocessor
        -W4 # ~~ -Wall -Wextra
    )
endif()

set(SPSX_LDFLAGS
    -fsanitize=address,undefined
)

add_subdirectory(util)
add_subdirectory(gdb)
add_subdirectory(core)
