cmake_minimum_required(VERSION 3.20)

# Версия приходит из CI через -DAPP_VERSION="X.Y.Z"
if(NOT DEFINED APP_VERSION)
    set(APP_VERSION "0.0.1")
endif()
string(REPLACE "." "," APP_VERSION_COMMA "${APP_VERSION}")

project(YawaFPSHUD VERSION ${APP_VERSION} LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Оптимизация под минимальный размер и нагрузку ──────────────
if(MSVC)
    add_compile_options(/O2 /GL /GS- /Oi /Ot /W3 /permissive- /utf-8)
    add_link_options(/LTCG /OPT:REF /OPT:ICF)
endif()

set(SOURCES
    src/main.cpp
    src/config/Model.cpp
    src/core/Sensors.cpp
    src/overlay/Overlay.cpp
    src/overlay/ConfigUI.cpp
    src/utils/Logger.h
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc.in
    ${CMAKE_CURRENT_BINARY_DIR}/resource.rc
    @ONLY
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resource.rc)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    APP_VERSION="${APP_VERSION}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d2d1 dwrite dxgi pdh dwmapi
    advapi32 user32 gdi32 shell32 ole32
)

# Манифест (DPI awareness) встраивается через НАШ resource.rc —
# отключаем автоматическое встраивание линкером, иначе CVT1100 duplicate
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/MANIFEST:NO")
endif()

install(FILES $<TARGET_FILE:${PROJECT_NAME}> DESTINATION .)
