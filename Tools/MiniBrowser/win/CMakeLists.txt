set(MiniBrowser_PRIVATE_INCLUDE_DIRECTORIES
    ${CMAKE_BINARY_DIR}
)

set(MiniBrowser_SOURCES
    BrowserWindow.cpp
    Common.cpp
    Common2.cpp
    MainWindow.cpp
    MiniBrowser.exe.manifest
    MiniBrowserLib.rc
    WebKitBrowserWindow.cpp
    WinMain.cpp
    stdafx.cpp
)

set(MiniBrowser_PRIVATE_DEFINITIONS _UNICODE)
set(MiniBrowser_PRIVATE_LIBRARIES
    comctl32
    shlwapi
    user32
)
set(MiniBrowser_FRAMEWORKS
    JavaScriptCore
    PAL
    WTF
    WebCore
    WebKit
)

add_library(MiniBrowserInjectedBundle SHARED
    Common2.cpp
    InjectedBundle.cpp
)
target_link_libraries(MiniBrowserInjectedBundle WebKit::WebKit)
target_include_directories(MiniBrowserInjectedBundle PRIVATE ${CMAKE_BINARY_DIR})

WEBKIT_EXECUTABLE_DECLARE(MiniBrowser)

# LuaJIT bridge (optional). Enabled only when the build environment provides LUAJIT_DIR
# pointing at a directory containing lua.hpp and lua51.lib (set by the CI workflow).
if(DEFINED ENV{LUAJIT_DIR} AND EXISTS "$ENV{LUAJIT_DIR}/lua51.lib")
    list(APPEND MiniBrowser_SOURCES MiniBrowserLua.cpp)
endif()

WEBKIT_EXECUTABLE(MiniBrowser)

if(DEFINED ENV{LUAJIT_DIR} AND EXISTS "$ENV{LUAJIT_DIR}/lua51.lib")
    target_include_directories(MiniBrowser PRIVATE "$ENV{LUAJIT_DIR}")
    target_link_libraries(MiniBrowser PRIVATE "$ENV{LUAJIT_DIR}/lua51.lib")
    target_compile_definitions(MiniBrowser PRIVATE MINIBROWSER_LUAJIT)
endif()

set_target_properties(MiniBrowser PROPERTIES WIN32_EXECUTABLE ON)
