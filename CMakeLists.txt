cmake_minimum_required(VERSION 3.20)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_ARCHITECTURES arm64)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
endif()

project(maayavi C)

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

set(GAME_SOURCES
    src/main.c
    src/game.c
    src/render.c
    src/sprites.c
    src/nk_sdl3.c
    src/ui.c
)

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    add_executable(maayavi MACOSX_BUNDLE ${GAME_SOURCES})
    target_link_libraries(maayavi PRIVATE SDL3::SDL3-static)

    set_target_properties(maayavi PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.maayavi.game"
        MACOSX_BUNDLE_BUNDLE_NAME "Maayavi"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.maayavi.game"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
        XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "37353G9J56"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
        XCODE_GENERATE_SCHEME TRUE
        RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard"
    )

    target_sources(maayavi PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/LaunchScreen.storyboard
        PROPERTIES MACOSX_PACKAGE_LOCATION Resources
    )
else()
    add_executable(maayavi ${GAME_SOURCES})
    target_link_libraries(maayavi PRIVATE SDL3::SDL3)

    if(EMSCRIPTEN)
        set(CMAKE_EXECUTABLE_SUFFIX ".html")
        target_link_options(maayavi PRIVATE
            -sALLOW_MEMORY_GROWTH=1
            -sINITIAL_MEMORY=33554432
            -sFULL_ES2=1
            "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
        )
    endif()
endif()

target_include_directories(maayavi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
