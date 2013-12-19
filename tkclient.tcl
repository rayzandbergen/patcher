load ./libtk_client.so Patcher
loadxml

proc show { trackIdx sectionIdx } {
    set trackName [xmlget track $trackIdx name]
    set nofSections [xmlget track $trackIdx section max]
    puts "$trackName has $nofSections sections"
    set sectionName [xmlget track $trackIdx section $sectionIdx name]
    puts "   $sectionName"
    set nofParts [xmlget track $trackIdx section $sectionIdx part max]
    for {set partIdx 0} {$partIdx < $nofParts} {incr partIdx} {
        set partName [xmlget track $trackIdx section $sectionIdx \
                        part $partIdx name]
        puts "        $partName"
    }
}
proc GetData {chan} {
    global packetCount metaMode currentTrack currentSection \
            trackIdxWithinSet type deviceId part midi1 midi2 midi3
    if {![eof $chan]} {
        scan [gets $chan] "%x %x %x %x %x %x %x %x %x %x %x" \
            packetCount metaMode currentTrack currentSection \
            trackIdxWithinSet type deviceId part midi1 midi2 midi3
        puts "********* $packetCount **********"
        show $currentTrack $currentSection
    }
}

set chan [ open "|./stdout_client" ]
fileevent $chan readable [list GetData $chan]
vwait packetCount

set nofTracks [xmlget track max]
for {set trackIdx 0} {$trackIdx < $nofTracks} {incr trackIdx} {
    set trackName [xmlget track $trackIdx name]
    set nofSections [xmlget track $trackIdx section max]
    puts "$trackName has $nofSections sections"
    for {set sectionIdx 0} {$sectionIdx < $nofSections} {incr sectionIdx} {
        set sectionName [xmlget track $trackIdx section $sectionIdx name]
        puts "   $sectionName"
        set nofParts [xmlget track $trackIdx section $sectionIdx part max]
        for {set partIdx 0} {$partIdx < $nofParts} {incr partIdx} {
            set partName [xmlget track $trackIdx section $sectionIdx \
                            part $partIdx name]
            puts "        $partName"
        }
    }
}

vwait forever
