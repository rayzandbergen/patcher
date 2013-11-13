/*! \file xml.h
 *  \brief Contains functions to read track definitions from an XML file, and clean them up again.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef XML_H
#define XML_H
#include "trackdef.h"
extern int importTracks(const char *inFile, std::vector<Track*> &tracks, SetList &setList);
extern void clearTracks(std::vector<Track*> &trackList);
#endif
