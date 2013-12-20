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

proc ProcessTrackChange { newTrack } {
    set ::State::currentTrack $newTrack
    set name [ xmlget track $::State::currentTrack name ]
    set n [ xmlget track max ]
    set i [ expr $::State::currentTrack + 1 ]
    .currentFrame.track configure -text "track $i/$n $name"
    Fantom::Refresh
}

proc ProcessSectionChange { newSection } {
    set ::State::currentSection $newSection
    set name [ xmlget track $::State::currentTrack \
               section $::State::currentSection name ]
    set n [ xmlget track $::State::currentTrack section max ]
    set i [ expr $::State::currentSection + 1 ]
    .currentFrame.section configure -text "section $i/$n $name"
    Section::Refresh
}

proc ProcessPatcherEvent { line } {
    global eventMembers
    eval "scan \"$line\" \"%x %x %x %x %x %x %x %x %x %x %x\" $eventMembers"
    LogEvent $line
    if { $currentTrack != $::State::currentTrack } {
        ProcessTrackChange $currentTrack
    }
    if { $currentSection != $::State::currentSection } {
        ProcessSectionChange $currentSection
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

namespace eval Section {
    proc Create { } {
        canvas .c -width 30c -height 5c
        for {set i 0} {$i < 8} {incr i} {
            set row [ expr ( $i % 4 ) * 30 + 30 ]
            set col [ expr ( $i / 4 ) * 180 + 30 ]
            lappend ::Section::numberId [ .c create text ${col}p ${row}p -text 0 ]
            set col [ expr $col + 20 ]
            lappend ::Section::channelId [ .c create text ${col}p ${row}p -text 0 ]
            set col [ expr $col + 40 ]
            lappend ::Section::nameId [ .c create text ${col}p ${row}p -text {<name>} ]
        }
        return .c
    }
    proc Refresh { } {
        set nofParts [ xmlget track $::State::currentTrack section $::State::currentSection \
                                   part max ]
        for {set i 0} {$i < 8} {incr i} {
            set id [ lindex $::Section::numberId $i ]
            .c dchars $id 0 end
            set id [ lindex $::Section::channelId $i ]
            .c dchars $id 0 end
            set id [ lindex $::Section::nameId $i ]
            .c dchars $id 0 end
            if { $i < $nofParts } {
                set name [ xmlget track $::State::currentTrack section $::State::currentSection \
                            part $i name ]
                set channel [ xmlget track $::State::currentTrack section $::State::currentSection \
                               part $i channel ]
                set id [ lindex $::Section::numberId $i ]
                set num [ expr $i + 1 ]
                .c insert $id end $num
                set id [ lindex $::Section::channelId $i ]
                incr channel
                .c insert $id end $channel
                set id [ lindex $::Section::nameId $i ]
                .c insert $id end $name
            }
        }
    }
}

namespace eval Fantom {
    proc Create { } {
        canvas .fantomCanvas -width 30c -height 10c
        for {set i 0} {$i < 16} {incr i} {
            set preset [ xmlget track $::State::currentTrack \
                         performance part $i preset ]
            set name [ xmlget track $::State::currentTrack \
                       performance part $i patch name ]
            set row [ expr ( $i % 4 ) * 30 + 30 ]
            set col [ expr ( $i / 4 ) * 180 + 30 ]
            set num [ expr $i + 1 ]
            lappend ::Fantom::numberId [ .fantomCanvas create text ${col}p ${row}p -text $num ]
            set col [ expr $col + 40 ]
            lappend ::Fantom::presetId [ .fantomCanvas create text ${col}p ${row}p -text $preset ]
            set col [ expr $col + 80 ]
            lappend ::Fantom::nameId [ .fantomCanvas create text ${col}p ${row}p -text $name ]
        }
        return .fantomCanvas
    }
    proc Refresh { } {
        for {set i 0} {$i < 16} {incr i} {
            set preset [ xmlget track $::State::currentTrack \
                         performance part $i preset ]
            set name [ xmlget track $::State::currentTrack \
                       performance part $i patch name ]
            set id [ lindex $::Fantom::presetId $i ]
            .fantomCanvas dchars $id 0 end
            .fantomCanvas insert $id end $preset
            set id [ lindex $::Fantom::nameId $i ]
            .fantomCanvas dchars $id 0 end
            .fantomCanvas insert $id end $name
        }
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
pack [ Fantom::Create ]
pack [ Section::Create ]
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

ProcessTrackChange $::State::currentTrack
ProcessSectionChange $::State::currentSection
fileevent $channel readable [list GetPatcherEvent $channel]
