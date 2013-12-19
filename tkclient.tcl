load ./libtk_client.so Patcher
loadxml
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
            set partName [xmlget track $trackIdx section $sectionIdx part $partIdx name]
            puts "        $partName"
        }
    }
}
