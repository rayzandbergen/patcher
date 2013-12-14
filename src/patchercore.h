/*! \file patchercore.h
 *  \brief Contains global definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 *
 *  \mainpage Ray's MIDI patcher
 *
\section intro Introduction

This MIDI patcher is an application that is intended to run on a Raspberry Pi board.
It was implemented because my off-the-shelf MIDI controllers are not powerful enough
to manage all the different sound settings and keyboard layouts that I use in my setup.

\htmlonly
<div align='center'>
<embed src="midi-setup.svg" type="image/svg+xml"/>
</div>
\endhtmlonly

When playing a single track of the Noble Silence set list, a single Fantom Performance used.
A Performance is a full mix of at most 16 sounds plus a chain of effects.
During the track, different sounds need to be controlled or played at different times.
In practice, this means switching between multiple keyboard layouts.
In my setup, switching is controlled by an FCB1010 MIDI foot controller.

The goal of this MIDI patcher implementation is to create and manage keyboard layouts in live situations.

\section rolandSpeak Roland-speak: Performances, Parts, and Patches.

A Fantom perfomance contains 16 'parts'.
A part points to a 'patch', i.e. the actual sound, and a lot of parameters that determine how this sound is mixed, e.g. volume, pan and various effect chains.
In addition, a part determines the MIDI channel for a sound, a key range within this channel, an optional transposition, and some high level parameters to modify the patch.

\htmlonly
<div align='center'>
<embed src="fantom.svg" type="image/svg+xml"/>
</div>
\endhtmlonly

By convention, there is a single part listening on each MIDI channel, so Fantom part 1 listens of MIDI channel 1, and so on.
However, this application will handle multiple parts per channel correctly.
The one-part-for-each-channel convention makes it easier for humans to keep an overview of a track definitions, at the expense of slightly more MIDI traffic in case of patch layering.
For layered patches, MIDI messages must be sent on each channel of the targeted parts, even though this could be solved by having multiple parts listen on the same channel.

Note that the Fantom also has a 'patch' mode, in which it only plays a single sound.
This mode is not used.
The Fantom has a system setting which makes it boot into performance mode.
Indeed, this application will not start up correctly with the Fantom in patch mode.

\section tracksectionpart Tracks, Sections, and Parts

The patcher replaces Roland-speak with different abstractions.

A track is usually a single item of the Noble Silence setlist.
It maps naturally onto a Fantom Performance.

Within a track there are sections.
A section is a single keyboard layout.
This is the most important abstraction that is introduced by the patcher.
It frees the Fantom from having to manage splits and layers.

Finally, a section consists of parts. A part controls a single sound in the track mix. Parts are used to create keyboard splits (by limiting the key range of a part) and layers (by having overlapping key ranges for multiple parts). Parts in the patcher are roughly equivalent to parts in the Fantom. They are called \a SwPart to distinguish them from \a Fantom::Part.

\section metaMode Meta mode

In meta mode, keys on the A30 keyboard behave like buttons that control the patcher's behaviour.
In live situations, this is the mode that is used to select the next track from the setlist.

Meta mode is enabled by pressing the 'reverb' switch on the A30, or by pressing pedal 7 on the FCB1010.
Is is disabled by pressing the 'reverb' button again, or by pressing pedal 6.
- C4 Selects previous track in the set list.
- D4 Selects next track in the set list.
- E4 Toggles info mode, where the Fantom display is used to show the network interfaces of the Pi.
- C5 and up select a track directly.

\section sectionSwitching Switching between sections

Section switches are triggered by the FCB foot controller.

- If chaining is enabled for the current track (in "tracks.xml"), then pressing
pedal 1 or 2 selects the previous section, while pedals 3 to 5
select the next section.
It is possible to link to a section in a different track.
The correct performance will be selected automatically.
- If chaining is disabled, then pedals 1 to 5 select a section directly.

\section trackSwitching Switching between tracks

Switching between tracks is done in meta mode.

\section configuration Configuration

Configuration is read from an XML file. A schema XSD is provided.

\section processes Processes
The application consists of 3 processes.
- The patcher core, which reads and writes MIDI data, and generates patcher events.
- The curses client, which produces terminal screen output from patcher events.
- The patcher administrator, which creates and cleans up the two processes above, as well as the event queue.

\htmlonly
<div align='center'>
<embed src="processes.svg" type="image/svg+xml"/>
</div>
\endhtmlonly

This configuration decouples the MIDI event loop in the core from the screen update loop.
Since screen updates are less important than the responsiveness of the core, it is given a lower scheduling priority.

 */
#ifndef PATCHER_H
#define PATCHER_H
#ifdef SINGLE_PRECISION
typedef float Real;     //!< Double precision for PC, single precision for Pi.
#elif DOUBLE_PRECISION
typedef double Real;    //!< Double precision for PC, single precision for Pi.
#else
#error either SINGLE_PRECISION or DOUBLE_PRECISION must be defined
#endif
#define TRACK_DEF "tracks.xml"  //!< Config file name.
const unsigned char masterProgramChangeChannel = 0x0f; //!< MIDI channel to listen on for program changes that will be interpreted by this application.
#endif
