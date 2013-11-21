/*! \file xml.cpp
 *  \brief Contains functions to read track definitions from an XML file.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "xml.h"
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/util/XMLStringTokenizer.hpp>
#include <xercesc/util/XMLChar.hpp>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include "error.h"

XERCES_CPP_NAMESPACE_USE

namespace
{

/*! \brief This class template caches T objects that are created from string constants.
 *
    It is used to create cache classes for XMLCh strings and DOMXPathExpressions.
*/
template <class T> class XMLCache
{
    std::map<const char *, T *> m_map;  //!< Map of all cached T's.
protected:
    //! \brief Create a new T object.
    virtual T *create(const char *stringConstant, DOMDocument *doc = 0) = 0;
    //! \brief Release a T object.
    virtual void release(T *) = 0;
public:
    //! \brief Retrieve a T pointer from cache, or create a new one.
    T *operator()(const char *stringConstant, DOMDocument *doc = 0)
    {
        typename std::map<const char *, T *>::iterator i = m_map.find(stringConstant);
        if (i == m_map.end())
        {
            T *expensiveObject = create(stringConstant, doc);
            m_map[stringConstant] = expensiveObject;
            return expensiveObject;
        }
        return i->second;
    }
    //! \brief Release all the resources held by the cache.
    void clear()
    {
        for (typename std::map<const char *, T *>::iterator i = m_map.begin(); i != m_map.end(); i++)
        {
            release(i->second);
        }
    }
};

/*! \brief This class caches XMLCh strings to avoid repeated conversions of the same string constants.
 */
class XMLStringCache: public XMLCache<XMLCh>
{
public:
    virtual XMLCh *create(const char *stringConstant, DOMDocument *doc = 0)
    {
        (void)doc;
        return XMLString::transcode(stringConstant);
    }
    virtual void release(XMLCh *c)
    {
        XMLString::release(&c);
    }
};

//! \brief Static instance of \a XMLStringCache.
static XMLStringCache xmlStr;

/*! \brief This class caches DOMXPathExpression instances to avoid repeated constructions from the same staring constants.
 */
class XPathCache: public XMLCache<DOMXPathExpression>
{
public:
    virtual DOMXPathExpression *create(const char *stringConstant, DOMDocument *doc = 0)
    {
        return doc->createExpression(xmlStr(stringConstant), 0);
    }
    virtual void release(DOMXPathExpression *x)
    {
        delete x;
    }
};

//! \brief Static instance of \a XPathCache.
static XPathCache xPath;

//! \brief Find the first node that matches an XPath expression.
DOMNode *findNode(DOMDocument *doc, const DOMElement *node, const char *path)
{
    DOMXPathResult *r = xPath(path, doc)->evaluate(node, DOMXPathResult::FIRST_ORDERED_NODE_TYPE, 0);
    DOMNode *rv = r->getNodeValue();
    delete r;
    return rv;
}

//! \brief Find nodes that match an XPath expression.
DOMXPathResult *findNodes(DOMDocument *doc, const DOMElement *node, const char *path)
{
    return xPath(path, doc)->evaluate(node, DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0);
}

//! \brief Temporary stringstream.
std::stringstream stringStream;

//! \brief Put an XMLString into an std::stream
std::stringstream &xmlStream(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    stringStream.clear();
    stringStream << s;
    XMLString::release(&s);
    return stringStream;
}

//! \brief Put an XMLString into an std::string.
std::string xmlStdString(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    std::string stdString(s);
    XMLString::release(&s);
    return stdString;
}

//! \brief Parse a DOM document into a track list and a \a SetList.
void parseTracks(DOMDocument *doc, TrackList &trackList, SetList &setList)
{
    DOMElement *root = doc->getDocumentElement();
    DOMNode *trackDefNode = findNode(doc, root, "/tracks/trackDefinitions");
    DOMXPathResult *trackNodes = findNodes(doc, (DOMElement*)trackDefNode, "./track");
    for (size_t trackIdx = 0; trackNodes->snapshotItem(trackIdx); trackIdx++)
    {
        DOMNode *trackNode = trackNodes->getNodeValue();
        char *name = XMLString::transcode(((DOMElement*)trackNode)->getAttribute(xmlStr("name")));
        Track *track = new Track(strdup(name));
        XMLString::release(&name);
        xmlStream(((DOMElement*)trackNode)->getAttribute(xmlStr("startSection")))
                >> track->m_startSection;
        if (xmlStdString(((DOMElement*)trackNode)->getAttribute(xmlStr("chain"))) == "true")
        {
            track->m_chain = true;
        }
        DOMXPathResult *sectionNodes = findNodes(doc, (DOMElement*)trackNode, "./section");
        for (size_t sectionIdx = 0; sectionNodes->snapshotItem(sectionIdx); sectionIdx++)
        {
            DOMNode *sectionNode = sectionNodes->getNodeValue();
            name = XMLString::transcode(((DOMElement*)sectionNode)->getAttribute(xmlStr("name")));
            Section *section = new Section(strdup(name));
            XMLString::release(&name);
            DOMNode *noteOffNode = findNode(doc, (DOMElement*)sectionNode, "./noteOff");
            if (noteOffNode)
            {
                if (((DOMElement*)noteOffNode)->hasAttribute(xmlStr("enter")))
                {
                    std::string no = xmlStdString(((DOMElement*)noteOffNode)->getAttribute(xmlStr("enter")));
                    section->m_noteOffEnter = no == "true";
                }
                if (((DOMElement*)noteOffNode)->hasAttribute(xmlStr("leave")))
                {
                    std::string no = xmlStdString(((DOMElement*)noteOffNode)->getAttribute(xmlStr("leave")));
                    section->m_noteOffLeave = no == "true";
                }
            }
            DOMNode *chainNode = findNode(doc, (DOMElement*)sectionNode, "./chain");
            if (chainNode)
            {
                const char *ss[] = {"nextSection", "previousSection"};
                for (size_t i=0; i<2; i++)
                {
                    if (((DOMElement*)chainNode)->hasAttribute(xmlStr(ss[i])))
                    {
                        int s = 0;
                        std::string trackStr = xmlStdString(((DOMElement*)chainNode)->getAttribute(xmlStr(ss[i])));
                        if (trackStr == "last")
                            s = TrackDef::LastSection;
                        else
                        {
                            stringStream.clear();
                            stringStream << trackStr;
                            stringStream >> s;
                        }
                        if (i == 0)
                            section->m_nextSection = s;
                        else
                            section->m_previousSection = s;
                    }
                }
                const char *ts[] = {"nextTrack", "previousTrack"};
                for (size_t i=0; i<2; i++)
                {
                    if (((DOMElement*)chainNode)->hasAttribute(xmlStr(ts[i])))
                    {
                        int t = 0;
                        std::string trackStr = xmlStdString(((DOMElement*)chainNode)->getAttribute(xmlStr(ts[i])));
                        if (trackStr == "next")
                            t = TrackDef::NextTrack;
                        else if (trackStr == "current")
                            t = TrackDef::CurrentTrack;
                        else if (trackStr == "previous")
                            t = TrackDef::PreviousTrack;
                        else
                        {
                            stringStream.clear();
                            stringStream << trackStr;
                            stringStream >> t;
                        }
                        if (i == 0)
                            section->m_nextTrack = t;
                        else
                            section->m_previousTrack = t;
                    }
                }
            }
            DOMXPathResult *partNodes = findNodes(doc, (DOMElement*)sectionNode, "./part");
            for (size_t partIdx = 0; partNodes->snapshotItem(partIdx); partIdx++)
            {
                DOMNode *partNode = partNodes->getNodeValue();
                SwPart *part;
                {
                    name = XMLString::transcode(((DOMElement*)partNode)->getAttribute(xmlStr("name")));
                    part = new SwPart(strdup(name));
                    XMLString::release(&name);
                }
                section->m_partList.push_back(part);
                int channel;
                xmlStream(((DOMElement*)partNode)->getAttribute(xmlStr("channel")))
                    >> channel;
                part->m_channel = channel-1;
                if (xmlStdString(((DOMElement*)partNode)->getAttribute(xmlStr("toggle"))) == "true")
                {
                    part->m_toggler.enable();
                }
                if (xmlStdString(((DOMElement*)partNode)->getAttribute(xmlStr("mono"))) == "true")
                {
                    part->m_mono = true;
                }
                if (((DOMElement*)partNode)->hasAttribute(xmlStr("sustainTranspose")))
                {
                    int tp = 0;
                    xmlStream(((DOMElement*)partNode)->getAttribute(xmlStr("sustainTranspose"))) >> tp;
                    part->m_transposer = new Transposer(tp);
                }
                DOMNode *controllerRemapNode = findNode(doc, (DOMElement*)partNode, "./controllerRemap");
                if (controllerRemapNode)
                {
                    delete part->m_controllerRemap; // default
                    std::string id = xmlStdString(((DOMElement*)controllerRemapNode)->getAttribute(xmlStr("id")));
                    if (id == "volQuadratic")
                        part->m_controllerRemap = new ControllerRemap::VolQuadratic;
                    else if (id == "volReverse")
                        part->m_controllerRemap = new ControllerRemap::VolReverse;
                    else
                        throw(Error("unknown controllerRemap id"));
                }
                if (!part->m_controllerRemap)
                    part->m_controllerRemap = new ControllerRemap::Default;

                DOMNode *rangeNode = findNode(doc, (DOMElement*)partNode, "./range");
                if (rangeNode)
                {
                    if (((DOMElement*)rangeNode)->hasAttribute(xmlStr("lower")))
                    {
                        std::string note = xmlStdString(((DOMElement*)rangeNode)->getAttribute(xmlStr("lower")));
                        part->m_rangeLower = SwPart::stringToNoteNum(note.c_str());
                    }
                    if (((DOMElement*)rangeNode)->hasAttribute(xmlStr("upper")))
                    {
                        std::string note = xmlStdString(((DOMElement*)rangeNode)->getAttribute(xmlStr("upper")));
                        part->m_rangeUpper = SwPart::stringToNoteNum(note.c_str());
                    }
                }
                DOMNode *transposeNode = findNode(doc, (DOMElement*)partNode, "./transpose");
                if (transposeNode)
                {
                    if (((DOMElement*)transposeNode)->hasAttribute(xmlStr("offset")))
                    {
                        xmlStream(((DOMElement*)transposeNode)->getAttribute(xmlStr("offset")))
                            >> part->m_transpose;
                    }
                    DOMNode *customNode = findNode(doc, (DOMElement*)transposeNode, "./custom");
                    if (customNode)
                    {
                        part->m_customTransposeEnabled = true;
                        memset(part->m_customTranspose, 0, sizeof(part->m_customTranspose));
                        if (((DOMElement*)customNode)->hasAttribute(xmlStr("offset")))
                        {
                            xmlStream(((DOMElement*)customNode)->getAttribute(xmlStr("offset")))
                                >> part->m_customTransposeOffset;
                        }
                        DOMXPathResult *mapNodes = findNodes(doc, (DOMElement*)customNode, "./map");
                        for (size_t mapIdx = 0; mapNodes->snapshotItem(mapIdx); mapIdx++)
                        {
                            DOMNode *mapNode = mapNodes->getNodeValue();
                            int from;
                            xmlStream(((DOMElement*)mapNode)->getAttribute(xmlStr("from"))) >> from;
                            from = (from + 120) % 12;
                            int to;
                            xmlStream(((DOMElement*)mapNode)->getAttribute(xmlStr("to"))) >> to;
                            part->m_customTranspose[from] = to;
                        }
                        mapNodes->release();
                    }
                }
            }
            track->addSection(section);
            partNodes->release();
        }
        trackList.push_back(track);
        sectionNodes->release();
    }
    trackNodes->release();
    DOMNode *setlistNode = findNode(doc, root, "/tracks/setList");
    trackNodes = findNodes(doc, (DOMElement*)setlistNode, "./track");
    for (size_t trackIdx = 0; trackNodes->snapshotItem(trackIdx); trackIdx++)
    {
        DOMNode *trackNode = trackNodes->getNodeValue();
        char *name = XMLString::transcode(((DOMElement*)trackNode)->getAttribute(xmlStr("name")));
        setList.add(trackList, name);
        XMLString::release(&name);
    }
    trackNodes->release();
}

} // anonymous namespace

//! \brief Parse an XML file into a track list and a \a SetList.
int importTracks (const char *inFile, TrackList &tracks, SetList &setList)
{
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        Error e("XMLException during init: ");
        char* message = XMLString::transcode(toCatch.getMessage());
        e.stream() << message;
        XMLString::release(&message);
        throw(e);
    }

    XercesDOMParser* parser = new XercesDOMParser();
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    //parser->setDoNamespaces(true);

    ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
    parser->setErrorHandler(errHandler);

    try {
        parser->parse(inFile);
        DOMDocument *doc = parser->getDocument();
        parseTracks(doc, tracks, setList);
    }
    catch (const XMLException& toCatch) {
        Error e("XMLException: ");
        char* message = XMLString::transcode(toCatch.getMessage());
        e.stream() << message;
        XMLString::release(&message);
        throw(e);
    }
    catch (const DOMException& toCatch) {
        Error e("DOMException: ");
        char* message = XMLString::transcode(toCatch.msg);
        e.stream() << message;
        XMLString::release(&message);
        throw(e);
    }
    catch (...) {
        throw(Error("xerces-c: unexpected exception"));
    }

    delete parser;
    delete errHandler;
    xmlStr.clear();
    xPath.clear();
    XMLPlatformUtils::Terminate();
    fixChain(tracks);
    return 0;
}

