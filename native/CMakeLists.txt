cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED APP_VERSION)
    set(APP_VERSION "0.0.2")
endif()
string(REPLACE "." "," APP_VERSION_COMMA "${APP_VERSION}")

project(YawaFPSHUD VERSION ${APP_VERSION} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /permissive- /utf-8)
    add_compile_options($<$<CONFIG:Release,RelWithDebInfo>:/O2>
                       $<$<CONFIG:Release,RelWithDebInfo>:/GL>
                       $<$<CONFIG:Release,RelWithDebInfo>:/GS->
                       $<$<CONFIG:Release,RelWithDebInfo>:/Oi>
                       $<$<CONFIG:Release,RelWithDebInfo>:/Ot>)
    add_link_options($<$<CONFIG:Release,RelWithDebInfo>:/LTCG>
                     $<$<CONFIG:Release,RelWithDebInfo>:/OPT:REF>
                     $<$<CONFIG:Release,RelWithDebInfo>:/OPT:ICF>)
endif()

# ── Иконка ───────────────────────────────────────────────────────────────────
# app.ico генерируется скриптом generate_ico.cmake через cmake -P
# Если файл уже есть (коммит или предыдущий запуск) — пропускаем.
set(ICO_OUT "${CMAKE_CURRENT_SOURCE_DIR}/resources/app.ico")
if(NOT EXISTS "${ICO_OUT}")
    message(STATUS "Generating app.ico via cmake -P...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -DICO_OUT=${ICO_OUT}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_ico.cmake
        RESULT_VARIABLE _ico_rc
    )
    if(NOT _ico_rc EQUAL 0)
        message(FATAL_ERROR
            "Failed to generate app.ico (exit ${_ico_rc}).\n"
            "Run manually: cmake -DICO_OUT=native/resources/app.ico -P native/generate_ico.cmake")
    endif()
else()
    message(STATUS "app.ico found: ${ICO_OUT}")
endif()

# ── resource.rc ──────────────────────────────────────────────────────────────
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/resource.rc
    @ONLY
)

# ── Executable ────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/config/Model.cpp
    src/core/Sensors.cpp
    src/overlay/Overlay.cpp
    src/overlay/ConfigUI.cpp
    src/overlay/ConfigPanelWeb.cpp
    src/utils/Logger.h
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resource.rc)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    APP_VERSION="${APP_VERSION}"
)

# WebView2 SDK обычно устанавливается через NuGet в CI
# Для локальной сборки нужно скачать Microsoft.Web.WebView2
# ── WebView2 SDK ──────────────────────────────────────────────────────────────
# Путь приходит из CI: -DWEBVIEW2_ROOT=<путь к Microsoft.Web.WebView2.X.Y.Z>
if(NOT DEFINED WEBVIEW2_ROOT OR WEBVIEW2_ROOT STREQUAL "")
    # Попытка найти локально (если разработчик установил через nuget)
    file(GLOB _WV2_CANDIDATES "${CMAKE_CURRENT_SOURCE_DIR}/../wv2/Microsoft.Web.WebView2.*")
    if(_WV2_CANDIDATES)
        list(GET _WV2_CANDIDATES 0 WEBVIEW2_ROOT)
    endif()
endif()

if(WEBVIEW2_ROOT)
    message(STATUS "WebView2 SDK: ${WEBVIEW2_ROOT}")
    set(WV2_INCLUDE "${WEBVIEW2_ROOT}/build/native/include")
    set(WV2_LIB    "${WEBVIEW2_ROOT}/build/native/x64/WebView2LoaderStatic.lib")
    if(NOT EXISTS "${WV2_INCLUDE}/WebView2.h")
        message(FATAL_ERROR "WebView2.h not found in ${WV2_INCLUDE}")
    endif()
    target_include_directories(${PROJECT_NAME} PRIVATE "${WV2_INCLUDE}")
    target_link_libraries(${PROJECT_NAME} PRIVATE "${WV2_LIB}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WEBVIEW2=1)
else()
    message(STATUS "WebView2 SDK not found — ConfigPanelWeb будет заглушкой")
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_WEBVIEW2=0)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1 dwrite dxgi pdh dwmapi powrprof
    advapi32 user32 gdi32 shell32 ole32 version.lib
)

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

install(FILES $<TARGET_FILE:${PROJECT_NAME}> DESTINATION .)
