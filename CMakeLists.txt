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
    src/activity.cpp
    src/controller.cpp
    src/dump.cpp
    src/fantomdef.cpp
    src/fantomdriver.cpp
    src/fantomscroller.cpp
    src/mididriver.cpp
    src/mididef.cpp
    src/monofilter.cpp
    src/networkif.cpp
    src/patcher.cpp
    src/persistent.cpp
    src/queue.cpp
    src/screen.cpp
    src/sequencer.cpp
    src/timer.cpp
    src/timestamp.cpp
    src/toggler.cpp
    src/trackdef.cpp
    src/transposer.cpp
#    src/undupparts.cpp
    src/xml.cpp
    now.h
)

set(queue_listenerSources
    src/activity.cpp
    src/controller.cpp
    src/dump.cpp
    src/fantomdef.cpp
    src/mididef.cpp
    src/monofilter.cpp
    src/queue.cpp
    src/queue_listener.cpp
    src/screen.cpp
    src/timer.cpp
    src/timestamp.cpp
    src/toggler.cpp
    src/trackdef.cpp
    src/xml.cpp
)
add_executable(patcher ${patcherSources})
add_executable(queue_listener ${queue_listenerSources})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/now.h
    COMMAND ./stamp.sh ${CMAKE_CURRENT_BINARY_DIR}/now.h
)

add_custom_target(tags DEPENDS ${patcherSources} COMMAND ctags -R)

add_dependencies(patcher tags)

set(patcherlibs "-lrt -lxerces-c -lasound")
set(queue_listenerlibs "-lrt -lxerces-c")

if(RASPBIAN)
    set(patcherlibs "${patcherlibs} -lncurses")
    set(queue_listenerlibs "${queue_listenerlibs} -lncurses")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DRASPBIAN")
else()
    set(patcherlibs "${patcherlibs} -lcurses")
    set(queue_listenerlibs "${queue_listenerlibs} -lcurses")
endif()

if (mudflap)
    set(patcherlibs "${patcherlibs} -lmudflap")
endif()

set_target_properties(patcher PROPERTIES LINK_FLAGS ${patcherlibs})
set_target_properties(queue_listener PROPERTIES LINK_FLAGS ${queue_listenerlibs})

