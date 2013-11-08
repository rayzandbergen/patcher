#ifndef XML_H
#define XML_H
#include "trackdef.h"
// #define EXPORT_TRACKS

#ifdef EXPORT_TRACKS
extern void exportTracks(const std::vector<Track*> &tracks, const char *filename);
#endif

extern int importTracks (const char *inFile, std::vector<Track*> &tracks);
extern void clearTracks(std::vector<Track*> &trackList);
#endif
