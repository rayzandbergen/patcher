/*! \file mididef.h
 *  \brief Contains a list of all MIDI notes wrapped in an enum.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef MIDI_DEF_H
#define MIDI_DEF_H
namespace Midi {
    //! \brief Names for all permissible MIDI notes.
    namespace Note {
        //! \brief Enum for all permissible MIDI notes.
        enum {
            /*   0 */ C0,
            /*   1 */ Db0,
            /*   2 */ D0,
            /*   3 */ Eb0,
            /*   4 */ E0,
            /*   5 */ F0,
            /*   6 */ Gb0,
            /*   7 */ G0,
            /*   8 */ Ab0,
            /*   9 */ A0,
            /*  10 */ Bb0,
            /*  11 */ B0,
            /*  12 */ C1,
            /*  13 */ Db1,
            /*  14 */ D1,
            /*  15 */ Eb1,
            /*  16 */ E1,
            /*  17 */ F1,
            /*  18 */ Gb1,
            /*  19 */ G1,
            /*  20 */ Ab1,
            /*  21 */ A1,
            /*  22 */ Bb1,
            /*  23 */ B1,
            /*  24 */ C2,
            /*  25 */ Db2,
            /*  26 */ D2,
            /*  27 */ Eb2,
            /*  28 */ E2,
            /*  29 */ F2,
            /*  30 */ Gb2,
            /*  31 */ G2,
            /*  32 */ Ab2,
            /*  33 */ A2,
            /*  34 */ Bb2,
            /*  35 */ B2,
            /*  36 */ C3,
            /*  37 */ Db3,
            /*  38 */ D3,
            /*  39 */ Eb3,
            /*  40 */ E3,
            /*  41 */ F3,
            /*  42 */ Gb3,
            /*  43 */ G3,
            /*  44 */ Ab3,
            /*  45 */ A3,
            /*  46 */ Bb3,
            /*  47 */ B3,
            /*  48 */ C4,
            /*  49 */ Db4,
            /*  50 */ D4,
            /*  51 */ Eb4,
            /*  52 */ E4,
            /*  53 */ F4,
            /*  54 */ Gb4,
            /*  55 */ G4,
            /*  56 */ Ab4,
            /*  57 */ A4,
            /*  58 */ Bb4,
            /*  59 */ B4,
            /*  60 */ C5,
            /*  61 */ Db5,
            /*  62 */ D5,
            /*  63 */ Eb5,
            /*  64 */ E5,
            /*  65 */ F5,
            /*  66 */ Gb5,
            /*  67 */ G5,
            /*  68 */ Ab5,
            /*  69 */ A5,
            /*  70 */ Bb5,
            /*  71 */ B5,
            /*  72 */ C6,
            /*  73 */ Db6,
            /*  74 */ D6,
            /*  75 */ Eb6,
            /*  76 */ E6,
            /*  77 */ F6,
            /*  78 */ Gb6,
            /*  79 */ G6,
            /*  80 */ Ab6,
            /*  81 */ A6,
            /*  82 */ Bb6,
            /*  83 */ B6,
            /*  84 */ C7,
            /*  85 */ Db7,
            /*  86 */ D7,
            /*  87 */ Eb7,
            /*  88 */ E7,
            /*  89 */ F7,
            /*  90 */ Gb7,
            /*  91 */ G7,
            /*  92 */ Ab7,
            /*  93 */ A7,
            /*  94 */ Bb7,
            /*  95 */ B7,
            /*  96 */ C8,
            /*  97 */ Db8,
            /*  98 */ D8,
            /*  99 */ Eb8,
            /* 100 */ E8,
            /* 101 */ F8,
            /* 102 */ Gb8,
            /* 103 */ G8,
            /* 104 */ Ab8,
            /* 105 */ A8,
            /* 106 */ Bb8,
            /* 107 */ B8,
            /* 108 */ C9,
            /* 109 */ Db9,
            /* 110 */ D9,
            /* 111 */ Eb9,
            /* 112 */ E9,
            /* 113 */ F9,
            /* 114 */ Gb9,
            /* 115 */ G9,
            /* 116 */ Ab9,
            /* 117 */ A9,
            /* 118 */ Bb9,
            /* 119 */ B9,
            /* 120 */ C10,
            /* 121 */ Db10,
            /* 122 */ D10,
            /* 123 */ Eb10,
            /* 124 */ E10,
            /* 125 */ F10,
            /* 126 */ Gb10,
            /* 127 */ G10,
            max
        };
    } // namespace Note

const int NofChannels = 16; //!< The number of MIDI channels.

//! \brief MIDI protocol: status bytes.
enum StatusByte
{
    noteOff = 0x80,
    noteOn = 0x90,
    aftertouch = 0xa0,
    controller = 0xb0,
    programChange = 0xc0,
    channelAftertouch = 0xd0,
    pitchBend = 0xe0,
    sysEx = 0xf0,
    tuneRequest = 0xf6,
    EOX = 0xf7,
    timingClock = 0xf8,
    realtimeStart = 0xfa,
    realtimeContinue = 0xfb,
    realtimeStop = 0xfc,
    activeSensing = 0xfe,
    sysReset = 0xff
};

//! \brief MIDI protocol: controller numbers.
enum DataByte
{
    continuous = 0,
    modulationWheel = 1,
    BCFSpinner1 = 1,
    dataEntry = 6,
    mainVolume = 7,
    continuousController16 = 16, //!< FCB1010 right pedal
    sustain = 0x40,
    BCFFader1 = 0x51,
    BCFFader8 = 0x58,
    BCFSwitchA = 0x59,
    effects1Depth = 0x5b, //!< Reverb depth
    dataEntryInc = 0x60,
    dataEntryDec = 0x61,
    resetAllControllers = 121,
    allNotesOff = 123,
    noData = 255
};

const char *noteName(uint8_t num);
void noteName(uint8_t num, char *s);
uint8_t noteValue(const char *s);
bool isNote(uint8_t statusByte);
bool isNoteOn(uint8_t statusByte, uint8_t data1, uint8_t data2);
bool isNoteOff(uint8_t statusByte, uint8_t data1, uint8_t data2);
bool isController(uint8_t statusByte);
uint8_t channel(uint8_t statusByte);
uint8_t status(uint8_t statusByte);

} // namespace Midi
#endif // MIDI_DEF_H
