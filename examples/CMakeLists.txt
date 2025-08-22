# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

project(dtkai_examples VERSION 1.0 LANGUAGES CXX)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Multimedia)
message("Using Qt version: ${QT_VERSION_MAJOR}")

# Add organized example directories
add_subdirectory(nlp)
add_subdirectory(vision)
add_subdirectory(speech)
add_subdirectory(other)

# Comprehensive presentation demo
add_executable(
    presentation_demo
    presentation_demo.cpp
)

target_link_libraries(
    presentation_demo PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Multimedia
    ${DtkCore_LIBRARIES}
    dtkai
)


