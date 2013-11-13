/*! \file xml.h
 *  \brief Contains functions to read track definitions from an XML file.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef XML_H
#define XML_H
#include "trackdef.h"
extern int importTracks(const char *inFile, TrackList &trackList, SetList &setList);
#endif
