#!/usr/bin/perl
use strict;
use warnings;
my @n = qw( C Db D Eb E F Gb G Ab A Bb B);
print qq@#ifndef MIDI_NOTE_H
#define MIDI_NOTE_H
namespace MidiNote {
    enum {
@;
for my $i (0 .. 127)
{
    printf("        /* %3d */ %s%s\n", $i, ($n[$i % 12] . int($i/12)), ($i < 127 ? ',' : ''));
}
print qq@
    };
};
#endif // MIDI_NOTE_H
@;
