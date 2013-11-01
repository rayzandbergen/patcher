#ifndef XML_H
#define XML_H
#include "trackdef.h"
extern void exportTracks(const std::vector<Track*> &tracks, const char *filename);
extern int importTracks (const char *inFile, std::vector<Track*> &tracks);
#endif
