set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SPSX_CXXFLAGS)
set(SPSX_LDFLAGS)

set(SPSX_CXXFLAGS_DEBUG)
set(SPSX_LDFLAGS_DEBUG)
set(SPSX_CXXFLAGS_RELEASE)
set(SPSX_LDFLAGS_RELEASE)

if (NOT WIN32) # Darwin/Linux
    list(APPEND SPSX_CXXFLAGS
        -Wall
        -Wextra
        -Wpedantic
        #-Wconversion
    )

    list(APPEND SPSX_CXXFLAGS_DEBUG
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
    )

    list(APPEND SPSX_LDFLAGS_DEBUG
        -fsanitize=address,undefined
    )

    list(APPEND SPSX_CXXFLAGS_RELEASE
        -march=native
    )

    list(APPEND SPSX_LDFLAGS_RELEASE
    )
else() # windows
    list(APPEND SPSX_CXXFLAGS
        -Zc:preprocessor # use standard compliant preprocessor
        -W4 # ~~ -Wall -Wextra
    )

    list(APPEND SPSX_CXXFLAGS_DEBUG
        -fsanitize=address
        -Zi
    )

    list(APPEND SPSX_LDFLAGS_DEBUG
        -fsanitize=address
    )

    list(APPEND SPSX_CXXFLAGS_RELEASE
    )

    list(APPEND SPSX_LDFLAGS_RELEASE
    )
endif()

list(APPEND SPSX_CXXFLAGS
    $<$<CONFIG:Debug>:${SPSX_CXXFLAGS_DEBUG}>
    $<$<CONFIG:Release>:${SPSX_CXXFLAGS_RELEASE}>
)
list(APPEND SPSX_LDFLAGS
    $<$<CONFIG:Debug>:${SPSX_LDFLAGS_DEBUG}>
    $<$<CONFIG:Release>:${SPSX_LDFLAGS_RELEASE}>
)

add_subdirectory(util)
add_subdirectory(gdb)
add_subdirectory(core)
