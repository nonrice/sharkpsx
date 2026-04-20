set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address")

add_executable(main
    main.cpp
    CPU.cpp
    RAM.cpp
    BUS.cpp
    System.cpp
    Debugger.cpp
)
