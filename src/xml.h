/*! \file xml.h
 *  \brief Contains functions to read track definitions from an XML file.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef XML_H
#define XML_H
#include <vector>
#include "trackdef.h"
#include "fantomdef.h"

class XML
{
    class XMLParser *m_xmlParser;
public:
    int importTracks (const char *inFile, TrackList &tracks, SetList &setList);
    int exportPerformances(const char *outFile, const Fantom::Performance *performanceList, 
            uint32_t nofPerformances);
    int importPerformances(const char *inFile, std::vector<Fantom::Performance*> &performanceList);
    XML();
    ~XML();
};

#endif
