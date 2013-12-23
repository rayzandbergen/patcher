#!/usr/bin/wish
load ./libtk_client.so Patcher

set eventMembers { packetCount metaMode currentTrack currentSection \
            trackIdxWithinSet type deviceId part \
            midi1 midi2 midi3
}

namespace eval State {
    foreach i $eventMembers {
        set $i -1
    }
}

proc LogEvent line {
    .eventLog insert end "$line\n"
    if { [ .eventLog count -lines 1.1 end ] > 5 } {
        .eventLog delete 1.1 2.1
    }
}

proc ProcessPatcherEvent { line } {
    global eventMembers
    eval "scan \"$line\" \"%x %x %x %x %x %x %x %x %x %x %x\" $eventMembers"
    processEvent $line
    LogEvent $line
    if { $currentTrack != $::State::currentTrack } {
        set ::State::currentTrack $currentTrack
    }
    if { $currentSection != $::State::currentSection } {
        set ::State::currentSection $currentSection
    }
}

# wait for first event
#set channel [ open "|./stdout_client -f" ]
set channel [ open "|./stdout_client" ]
set firstLine [ gets $channel ]
foreach i $eventMembers { lappend globals State::$i }
eval "scan \"$firstLine\" \"%x %x %x %x %x %x %x %x %x %x %x\" $globals"
loadxml

# get patcher event
proc GetPatcherEvent {channel} {
    if { ! [ eof $channel ] } {
        global eventMembers
        set line [ gets $channel ]
        ProcessPatcherEvent $line
    }
}

########### main #############
frame .viewOptionsFrame
pack .viewOptionsFrame -fill both -expand 1
checkbutton .viewOptionsFrame.cb1 -text "Fantom Parts" -command onClick  \
    -onvalue true -offvalue false -variable showFantomParts
checkbutton .viewOptionsFrame.cb2 -text "Section" -command onClick  \
    -onvalue true -offvalue false -variable showSection
checkbutton .viewOptionsFrame.cb3 -text "Event log" -command onClick  \
    -onvalue true -offvalue false -variable showEventLog
.viewOptionsFrame.cb2 select 
pack .viewOptionsFrame.cb1 -side left
pack .viewOptionsFrame.cb2 -side left
pack .viewOptionsFrame.cb3 -side left
frame .currentFrame
pack .currentFrame -fill both -expand 1
label .currentFrame.track -text {track x 1/1}
pack .currentFrame.track
label .currentFrame.section -text {section y 1/1}
pack .currentFrame.section
canvas .fantomCanvas -width 30c -height 10c
pack .fantomCanvas
canvas .c -width 30c -height 5c
pack .c
text .eventLog -width 40 -height 4
pack .eventLog

proc onClick {} {
    global showFantomParts
    if {$showFantomParts==true} {
        wm title . "y"
    } else {
        wm title . "x"
    }
}

wm title . "z"

fileevent $channel readable [list GetPatcherEvent $channel]

