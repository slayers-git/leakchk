# This file is a part of the leakchk library.
# Copyright (C) 2022 by Sergey Lafin
# Licensed under the LGPL v2.1, see the file LICENSE in base directory.

cmake_minimum_required (VERSION 3.12)
project (leakchk)

add_subdirectory (./csl)
include_directories (./csl/cthings)

if (UNIX OR CYGWIN OR MINGW)
    set (CMAKE_C_FLAGS_RELEASE "-O2")
    set (CMAKE_C_FLAGS_DEBUG   "-g -O0")
elseif (MSVC)
    set (CMAKE_C_FLAGS_RELEASE "/O2")
    set (CMAKE_C_FLAGS_DEBUG   "/g /O0")
endif ()

option (LC_DISABLE_LOCTRACKING "disable the tracking of the location where theallocation happened" OFF)
option (LC_DISABLE_MAGICCHECK "disable the magic number checks (protection from using the lc_* functions on the pointers allocated with regular malloc ())" OFF)
option (LC_DISABLE_THREADSAFETY "disable thread-safety with pthread_mutex'es" OFF)
option (LC_DISABLE_FUNCNAME "disable function name tracking" OFF)

if (LC_DISABLE_MAGICCHECK)
    add_compile_definitions (LC_DISABLE_MAGICCHECK=1)
endif ()

if (LC_DISABLE_LOCTRACKING)
    add_compile_definitions (LC_DISABLE_LOCATIONTRACKING=1)
endif ()

if (LC_DISABLE_FUNCNAME)
    add_compile_definitions (LC_DISABLE_FUNCNAME=1)
endif ()

if (LC_DISABLE_THREADSAFETY)
    add_compile_definitions (LC_DISABLE_THREADSAFETY=1)
endif ()

set (SRC
    ./leakchk.c)

set (HDR
    ./leakchk.h)

add_library (leakchk SHARED ${SRC})

install (FILES ${HDR} DESTINATION include)
install (TARGETS leakchk DESTINATION lib)
