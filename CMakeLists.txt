cmake_minimum_required(VERSION 3.20)
project(randomancer C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.x
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(randomancer
    src/main.c
    src/game.c
    src/render.c
    src/sprites.c
)

target_link_libraries(randomancer PRIVATE SDL3::SDL3)
target_include_directories(randomancer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(randomancer PROPERTIES
        LINK_FLAGS "-s ALLOW_MEMORY_GROWTH=1 -s INITIAL_MEMORY=33554432 --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
    )
endif()
