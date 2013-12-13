cmake_minimum_required(VERSION 2.8.4)

#set(CMAKE_BUILD_TYPE DEBUG)
set(CMAKE_BUILD_TYPE RELEASE)

project(patcher)

set(RASPBIAN 1)
#set(mudflap 1)

if (RASPBIAN)
    set(FEATURE_FLAGS "-DCURSES -DSINGLE_PRECISION")
    set(FEATURE_LIBS "-lcurses")
else()
    set(FEATURE_FLAGS "-DNCURSES -DDOUBLE_PRECISION")
    set(FEATURE_LIBS "-lncurses")
endif()
if (mudflap)
    set(FEATURE_FLAGS "${FEATURE_FLAGS} -fmudflap -DNO_TIMEOUT")
    set(FEATURE_LIBS "${FEATURE_LIBS} -lmudflap")
endif()

set(GCC_WARNINGS "-std=c++98 -pedantic -Wall -Wextra -Wshadow -Wstrict-aliasing=1")
set(CMAKE_CXX_FLAGS_DEBUG "${FEATURE_FLAGS} ${GCC_WARNINGS} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${FEATURE_FLAGS} ${GCC_WARNINGS} -O3")

include_directories(src ${CMAKE_CURRENT_BINARY_DIR})

set(patcher_coreSources
    src/activity.cpp
    src/controller.cpp
    src/fantomdef.cpp
    src/fantomdriver.cpp
    src/fantomscroller.cpp
    src/mididef.cpp
    src/mididriver.cpp
    src/monofilter.cpp
    src/networkif.cpp
    src/patchercore.cpp
    src/persistent.cpp
    src/queue.cpp
    src/screen.cpp
    src/sequencer.cpp
    src/timer.cpp
    src/timestamp.cpp
    src/toggler.cpp
    src/trackdef.cpp
    src/transposer.cpp
    src/xml.cpp
    now.h
)

set(curses_clientSources
    src/activity.cpp
    src/controller.cpp
    src/cursesclient.cpp
    src/fantomdef.cpp
    src/mididef.cpp
    src/monofilter.cpp
    src/queue.cpp
    src/screen.cpp
    src/timer.cpp
    src/timestamp.cpp
    src/toggler.cpp
    src/trackdef.cpp
    src/transposer.cpp
    src/xml.cpp
    now.h
)

set(patcherSources
    src/patcher.cpp
    src/queue.cpp
    now.h
)

add_executable(patcher_core ${patcher_coreSources})
add_executable(curses_client ${curses_clientSources})
add_executable(patcher ${patcherSources})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/now.h
    COMMAND ./stamp.sh ${CMAKE_CURRENT_BINARY_DIR}/now.h
)

add_custom_target(tags DEPENDS ${patcher_coreSources} COMMAND ctags -R)

add_dependencies(patcher_core tags)

set(patcher_corelibs "-lrt -lxerces-c -lasound ${FEATURE_LIBS}")
set(curses_clientlibs "-lrt -lxerces-c ${FEATURE_LIBS}")
set(patcherlibs "-lrt")

set_target_properties(patcher_core PROPERTIES LINK_FLAGS ${patcher_corelibs})
set_target_properties(curses_client PROPERTIES LINK_FLAGS ${curses_clientlibs})
set_target_properties(patcher PROPERTIES LINK_FLAGS ${patcherlibs})

