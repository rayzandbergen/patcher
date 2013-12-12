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

class XMLParser;
/*! \brief XML parser wrapper, hides \a XMLParser.
 *
 * It provides an API to
 * - read the \a TrackList and \a SetList,
 * - read the \a Fantom::PerformanceList from a cache file, and
 * - write the \a Fantom::PerformanceList to a cache file.
 */
class XML
{
    XMLParser *m_xmlParser; //!<    XML parser.
public:
    int importTracks (const char *inFile, TrackList &tracks, SetList &setList);
    int exportPerformances(const char *outFile, const Fantom::PerformanceList &performanceList);
    int importPerformances(const char *inFile, Fantom::PerformanceList &performanceList);
    XML();
    ~XML();
};

#endif
