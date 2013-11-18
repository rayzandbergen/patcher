#!/usr/bin/perl
use strict;
use warnings;
my @n = qw( C Db D Eb E F Gb G Ab A Bb B);
print qq^/*! \\file midi_note.h
 *  \\brief Contains a list of all MIDI notes wrapped in an enum.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen\@gmail.com)
 */
#ifndef MIDI_NOTE_H
#define MIDI_NOTE_H
namespace Midi {
    //! \\brief Names for all permissible MIDI notes.
    namespace Note {
        //! \\brief Enum for all permissible MIDI notes.
        enum {
^;
for my $i (0 .. 127)
{
    printf(
"            /* %3d */ %s,\n", $i, ($n[$i % 12] . int($i/12)));
}
print qq@            max
        };
    }
}
#endif // MIDI_NOTE_H
@;
