set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(main
    main.cpp
    CPU.cpp
    MemBlockDevice.cpp
    Bus.cpp
    System.cpp
    Debugger.cpp
)

target_compile_options(main PRIVATE
    -Wall
    -Wextra
    -fsanitize=address,undefined
    -g
    -fno-omit-frame-pointer
)

target_link_options(main PRIVATE
    -fsanitize=address,undefined
)
