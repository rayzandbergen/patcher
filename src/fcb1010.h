/*! \file fcb1010.h
 *  \brief Contains some FCB1010 definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef FCB1010_H
#define FCB1010_H
//! \brief FCB1010 definitions.
namespace FCB1010
{
    //! \brief MIDI program change numbers for foot switches.
    enum ProgramChange
    {
        FootSwitch1 = 0x00,
        FootSwitch2 = 0x01,
        FootSwitch3 = 0x02,
        FootSwitch4 = 0x03,
        FootSwitch5 = 0x04,
        FootSwitch6 = 0x05,
        FootSwitch7 = 0x06,
        FootSwitch8 = 0x07
    };
}
#endif // FCB1010_H
