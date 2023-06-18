cmake_minimum_required(VERSION 3.14)

project(cnetlib4 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

add_library(cnetlib4 SHARED
  cnetlib4_global.h
  cnetlib4.cpp
  cnetlib4.h

  cnetlib4_common.hpp

  message.h
  message.cpp

)

target_link_libraries(cnetlib4 PRIVATE Qt${QT_VERSION_MAJOR}::Core ws2_32 wsock32 stdc++fs gcutils)

target_compile_definitions(cnetlib4 PRIVATE CNETLIB4_LIBRARY)
