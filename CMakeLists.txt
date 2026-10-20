cmake_minimum_required(VERSION 3.21)
project(BingusNativeBridge VERSION 0.1.0 LANGUAGES C RC)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Bingus Native Bridge builds with MSVC for 64-bit Windows only.")
endif()

configure_file(src/bnb_version.h.in "${CMAKE_CURRENT_BINARY_DIR}/generated/bnb_version.h" @ONLY)

# The plugin: no C runtime, no entry point, no imports. /Brepro makes the
# output byte-for-byte reproducible; Release builds carry no debug paths.
add_library(bingus_native_bridge SHARED src/bridge.c src/bridge.rc)
set_target_properties(bingus_native_bridge PROPERTIES
    OUTPUT_NAME bingus_native_bridge_pluginw64_release
    MSVC_RUNTIME_LIBRARY MultiThreaded)
target_include_directories(bingus_native_bridge PRIVATE
    src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_options(bingus_native_bridge PRIVATE
    /std:c11 /W4 /WX /GS- /Zl /Brepro)
target_link_options(bingus_native_bridge PRIVATE
    /NOENTRY /NODEFAULTLIB /Brepro /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT)
# Runtime checks need the C runtime the plugin does not link.
string(REGEX REPLACE "/RTC[^ ]*" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

option(BNB_BUILD_TESTS "Build the offline mock-host tests (never distributed)" ON)
if(BNB_BUILD_TESTS)
    include(FetchContent)
    # Official Lua 5.1.4 source for the test host only. Point BNB_LUA_URL at a
    # local copy of the same archive to build offline; the hash still applies.
    set(BNB_LUA_URL "https://www.lua.org/ftp/lua-5.1.4.tar.gz" CACHE STRING
        "Lua 5.1.4 source archive for the test host")
    FetchContent_Declare(lua51
        URL "${BNB_LUA_URL}"
        URL_HASH SHA256=b038e225eaf2a5b57c9bcc35cd13aa8c6c8288ef493d52970c9545074098af3a
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(lua51)

    file(GLOB BNB_LUA_SOURCES "${lua51_SOURCE_DIR}/src/*.c")
    list(REMOVE_ITEM BNB_LUA_SOURCES
        "${lua51_SOURCE_DIR}/src/lua.c"
        "${lua51_SOURCE_DIR}/src/luac.c"
        "${lua51_SOURCE_DIR}/src/print.c")
    add_library(bnb_lua51 STATIC ${BNB_LUA_SOURCES})
    target_include_directories(bnb_lua51 PUBLIC "${lua51_SOURCE_DIR}/src")
    target_compile_definitions(bnb_lua51 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(bnb_lua51 PRIVATE /W1)

    add_executable(bnb_mock_host tests/mock_host.c)
    target_include_directories(bnb_mock_host PRIVATE src)
    target_link_libraries(bnb_mock_host PRIVATE bnb_lua51)
    target_compile_options(bnb_mock_host PRIVATE /W4 /WX)
    add_dependencies(bnb_mock_host bingus_native_bridge)

    enable_testing()
    add_test(NAME bridge_mock_host
        COMMAND bnb_mock_host "$<TARGET_FILE:bingus_native_bridge>"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_bridge.lua" "${PROJECT_VERSION}")
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
        add_test(NAME bridge_pe_audit
            COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/pe_audit.py"
                    "$<TARGET_FILE:bingus_native_bridge>" --version "${PROJECT_VERSION}")
    endif()
endif()
