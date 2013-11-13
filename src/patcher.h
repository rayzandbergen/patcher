/*! \file patcher.h
 *  \brief Contains global definitions.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef PATCHER_H
#define PATCHER_H
#ifdef RASPBIAN
typedef float Real;     //!< Double precision for PC, single precision for Pi.
#else
typedef double Real;    //!< Double precision for PC, single precision for Pi.
#endif
#define TRACK_DEF "tracks.xml"  //!< Config file name.
#endif
