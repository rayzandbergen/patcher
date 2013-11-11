#ifndef XML_H
#define XML_H
#include "trackdef.h"
extern int importTracks(const char *inFile, std::vector<Track*> &tracks);
extern void clearTracks(std::vector<Track*> &trackList);
#endif
