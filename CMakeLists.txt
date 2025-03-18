cmake_minimum_required(VERSION 3.14)

set(TARGET "qtzlgcanbus")
project(${TARGET} LANGUAGES CXX)
set(CMAKE_DEBUG_POSTFIX "d")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Gui LinguistTools SerialBus)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui LinguistTools SerialBus)

file(GLOB SRC_H "src/*.h")
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_FILES)
add_library(${TARGET} SHARED
    ${SRC_H}
    ${SRC_FILES}
    src/zlgcan.qrc
)

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/lib)

target_link_libraries(${TARGET} PRIVATE Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::SerialBus)

target_compile_options(${TARGET} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

