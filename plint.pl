#!/usr/bin/perl
use strict;
use warnings;
my $l = 'log.txt';
open(L, $l) or die "cannot open $l";
my $trackIdx;
my $trackName;
my $section;
my $swPartIdx;
my $hwPartIdx;
my $hwPartPreset;
my $keyRangeUpper;
my $fadeWidthUpper;
my $keyRangeLower;
my $fadeWidthLower;
while(<L>)
{
    chomp;
    if (m/^track(\d+)\.name:(.+)/)
    {
        $trackIdx = $1;
        $trackName = $2;
        print "$trackIdx: $trackName\n";
    }
    if (m/^track(\d+)\.section(\d+)\.swPart(\d+)\.hwPart(\d+)\.preset:(.+)/)
    {
        $trackIdx = $1;
        $section = $2;
        $swPartIdx = $3;
        $hwPartIdx = $4;
        $hwPartPreset = $5;
    }
    if (m/^track(\d+)\.section(\d+)\.swPart(\d+)\.hwPart(\d+)\.keyRangeUpper:(.+)/)
    {
        $keyRangeUpper = $5;
    }
    if (m/^track(\d+)\.section(\d+)\.swPart(\d+)\.hwPart(\d+)\.fadeWidthUpper:(.+)/)
    {
        $fadeWidthUpper = $5;
        if ($fadeWidthUpper > 0 && $keyRangeUpper eq 'G10')
        {
            print "** useless fadeWidthUpper: $trackIdx:$trackName, part $hwPartIdx:$hwPartPreset\n"
        }
        elsif ($fadeWidthUpper > 0)
        {
            print "possibly useful fadeWidthUpper: $trackIdx:$trackName, part $hwPartIdx:$hwPartPreset:$keyRangeUpper:$fadeWidthUpper\n"
        }
    }
}
