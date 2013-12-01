/*! \file patchercore.h
 *  \brief Contains global definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 *
 *  \mainpage Ray's MIDI patcher
 *
\section intro Introduction

\image html midi-setup.svg

When playing a single track of the Noble Silence set list, a single Fantom Performance used. A Performance is a full mix of at most 16 sounds plus a chain of effects.
During the track, different sounds need to be controlled or played at different times.
In practice, this means switching between multiple keyboard layouts.
In my setup, switching is controlled by an FCB1010 MIDI foot controller.

The goal of this MIDI patcher implementation is to create and manage keyboard layouts in live situations.

\section rolandSpeak Roland-speak

A Fantom perfomance contains 16 'parts'. A part points to a 'patch', i.e. the actual sound, and a lot of parameters that determine how this sound is mixed, e.g. volume, pan and various effect chains.
In addition, a part determines the MIDI channel for a sound, a key range within this channel, an optional transposition, and some high level parameters to modify the path.
By convention, there is a single part listening on each MIDI channel, so Fantom part 1 listens of MIDI channel 1, and so on. However, this application will handle multiple parts per channel correctly.
The one-part-for-each-channel convention makes it easier for humans to keep an overview of a track definitions, at the expense of slightly more MIDI traffic in case of patch layering.
For layered patches, MIDI messages must be sent on each channel of the targeted parts, even though this could be solved by having multiple parts listen on the same channel.

Note that the Fantom also has a 'patch' mode, in which it only plays a single sound. This mode is not used. The Fantom has a system setting which makes it boot into performance mode. Indeed, this application will not start up correctly with the Fantom in patch mode.

\section tracksectionpart Tracks, Sections, and Parts

A track is usually a single item of the Noble Silence setlist.
It maps naturally onto a Fantom Performance.

Within a track there are sections. A section is a single keyboard layout.

Finally, a section consists of parts. A part controls a single sound in the track mix. Parts are used to create keyboard splits (by limiting the key range of a part) and layers (by having overlapping key ranges for multiple parts). Parts in the patcher are roughly equivalent to parts in the Fantom. They are called \a SwPart to distinguish them from \a Fantom::Part.

\section metaMode Meta mode

In meta mode, keys on the keyboard behave like buttons that control the patcher's behaviour.

\section sectionSwitching Switching between sections

Section switches are triggered by the FCB foot controller.
\todo Explain chaining and direct selection.

\section trackSwitching Switching between tracks

Switching between tracks is done in meta mode.

\section configuration Configuration

Configuration is read from an XML file. A schema XSD is provided.

 */
#ifndef PATCHER_H
#define PATCHER_H
#ifdef RASPBIAN
typedef float Real;     //!< Double precision for PC, single precision for Pi.
#else
typedef double Real;    //!< Double precision for PC, single precision for Pi.
#endif
#define TRACK_DEF "tracks.xml"  //!< Config file name.
const unsigned char masterProgramChangeChannel = 0x0f; //!< MIDI channel to listen on for program changes that will be interpreted by this application.
#endif
