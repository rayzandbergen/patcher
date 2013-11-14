cmake_minimum_required(VERSION 2.8.4)

#set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

project(patcher)
set(RASPBIAN 1)
#set(mudflap 1)

set(GCC_WARNINGS "-std=c++98 -pedantic -Wall -Wextra -Wshadow -Wstrict-aliasing=1")
set(CMAKE_CXX_FLAGS_DEBUG "${GCC_WARNINGS} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${GCC_WARNINGS} -O3")
if (mudflap)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fmudflap -DNO_TIMEOUT")
endif()

include_directories(src ${CMAKE_CURRENT_BINARY_DIR})

set(patcherSources
    src/fantom.cpp
    src/monofilter.cpp
    src/patcher.cpp
    src/midi.cpp
    src/networkif.cpp
    src/toggler.cpp
    src/alarm.cpp
    src/xml.cpp
    src/dump.cpp
    src/activity.cpp
    src/trackdef.cpp
    src/sequencer.cpp
    src/controller.cpp
    src/screen.cpp
    src/transposer.cpp
    src/timestamp.cpp
    src/persistent.cpp
    now.h
)

add_executable(patcher ${patcherSources})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/now.h
    COMMAND ./stamp.sh ${CMAKE_CURRENT_BINARY_DIR}/now.h
)

add_custom_target(tags DEPENDS ${patcherSources} COMMAND ctags -R)

add_dependencies(patcher tags)

set(patcherlibs "-lrt -lxerces-c -lasound")

if(RASPBIAN)
    set(patcherlibs "${patcherlibs} -lncurses")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRASPBIAN")
else()
    set(patcherlibs "${patcherlibs} -lcurses")
endif()

if (mudflap)
    set(patcherlibs "${patcherlibs} -lmudflap")
endif()

set_target_properties(patcher PROPERTIES LINK_FLAGS ${patcherlibs})

