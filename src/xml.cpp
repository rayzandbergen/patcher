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
#include <list>
#include <map>
#include "error.h"

#ifdef FAKE_STL // set in PREDEFINED in doxygen config
namespace std { /*! \brief STL map */ template <class K, class V> class map {
        public T key; /*!< Key. */ V value; /*!< Value. */ }; }
#endif

XERCES_CPP_NAMESPACE_USE

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

/*! \brief This class caches DOMXPathExpression instances to avoid repeated constructions from the same staring constants.
 */
class XPathCache: public XMLCache<DOMXPathExpression>
{
    XMLStringCache *m_xmlStr;
public:
    XPathCache(XMLStringCache *sc): m_xmlStr(sc) { }
    virtual DOMXPathExpression *create(const char *stringConstant, DOMDocument *doc = 0)
    {
        return doc->createExpression((*m_xmlStr)(stringConstant), 0);
    }
    virtual void release(DOMXPathExpression *x)
    {
        delete x;
    }
};

class XMLParser
{
    std::stringstream m_stringStream;  //!< Temporary stringstream.
    XMLStringCache m_xmlStr;           //!< Instance of \a XMLStringCache.
    XPathCache m_xPath;                //!< Instance of \a XPathCache.
    DOMNode *findNode(DOMDocument *doc, const DOMElement *node, const char *path);
    DOMXPathResult *findNodes(DOMDocument *doc, const DOMElement *node, const char *path);
    std::stringstream &xmlStream(const XMLCh *c);
    std::string xmlStdString(const XMLCh *c);
    uint8_t toByte(const XMLCh *c);
    void serialise(DOMDocument *doc, const char *outFile);
    uint8_t getByteAttribute(DOMElement *node, const char *attr, int defaultByte = -1);
    void parseTracks(DOMDocument *doc, TrackList &trackList, SetList &setList);
    void parsePerformances(DOMDocument *doc, std::vector<Fantom::Performance*> &performanceList);
public:
    int importTracks (const char *inFile, TrackList &tracks, SetList &setList);
    int exportPerformances(const char *outFile, const Fantom::Performance *performanceList,
            uint32_t nofPerformances);
    int importPerformances(const char *inFile, std::vector<Fantom::Performance*> &performanceList);
    XMLParser();
    ~XMLParser();
};

XMLParser::XMLParser(): m_xPath(&m_xmlStr)
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
}

XMLParser::~XMLParser()
{
    m_xmlStr.clear();
    m_xPath.clear();
    XMLPlatformUtils::Terminate();
}

//! \brief Find the first node that matches an XPath expression.
DOMNode *XMLParser::findNode(DOMDocument *doc, const DOMElement *node, const char *path)
{
    DOMXPathResult *r = m_xPath(path, doc)->evaluate(node, DOMXPathResult::FIRST_ORDERED_NODE_TYPE, 0);
    DOMNode *rv = r->getNodeValue();
    delete r;
    return rv;
}

//! \brief Find nodes that match an XPath expression.
DOMXPathResult *XMLParser::findNodes(DOMDocument *doc, const DOMElement *node, const char *path)
{
    return m_xPath(path, doc)->evaluate(node, DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0);
}

//! \brief Put an XMLString into an std::stream
std::stringstream &XMLParser::xmlStream(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    m_stringStream.clear();
    m_stringStream << s;
    XMLString::release(&s);
    return m_stringStream;
}

//! \brief  Convert an XML string to a byte value.
uint8_t XMLParser::toByte(const XMLCh *c)
{
    int tmp;
    xmlStream(c) >> tmp;
    return (uint8_t)tmp;
}

//! \brief Put an XMLString into an std::string.
std::string XMLParser::xmlStdString(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    std::string stdString(s);
    XMLString::release(&s);
    return stdString;
}

//! \brief Parse a DOM document into a track list and a \a SetList.
void XMLParser::parseTracks(DOMDocument *doc, TrackList &trackList, SetList &setList)
{
    DOMElement *root = doc->getDocumentElement();
    DOMNode *trackDefNode = findNode(doc, root, "/tracks/trackDefinitions");
    DOMXPathResult *trackNodes = findNodes(doc, (DOMElement*)trackDefNode, "./track");
    for (size_t trackIdx = 0; trackNodes->snapshotItem(trackIdx); trackIdx++)
    {
        DOMNode *trackNode = trackNodes->getNodeValue();
        char *name = XMLString::transcode(((DOMElement*)trackNode)->getAttribute(m_xmlStr("name")));
        Track *track = new Track(strdup(name));
        XMLString::release(&name);
        xmlStream(((DOMElement*)trackNode)->getAttribute(m_xmlStr("startSection")))
                >> track->m_startSection;
        if (xmlStdString(((DOMElement*)trackNode)->getAttribute(m_xmlStr("chain"))) == "true")
        {
            track->m_chain = true;
        }
        DOMXPathResult *sectionNodes = findNodes(doc, (DOMElement*)trackNode, "./section");
        for (size_t sectionIdx = 0; sectionNodes->snapshotItem(sectionIdx); sectionIdx++)
        {
            DOMNode *sectionNode = sectionNodes->getNodeValue();
            name = XMLString::transcode(((DOMElement*)sectionNode)->getAttribute(m_xmlStr("name")));
            Section *section = new Section(strdup(name));
            XMLString::release(&name);
            DOMNode *noteOffNode = findNode(doc, (DOMElement*)sectionNode, "./noteOff");
            if (noteOffNode)
            {
                if (((DOMElement*)noteOffNode)->hasAttribute(m_xmlStr("enter")))
                {
                    std::string no = xmlStdString(((DOMElement*)noteOffNode)->getAttribute(m_xmlStr("enter")));
                    section->m_noteOffEnter = no == "true";
                }
                if (((DOMElement*)noteOffNode)->hasAttribute(m_xmlStr("leave")))
                {
                    std::string no = xmlStdString(((DOMElement*)noteOffNode)->getAttribute(m_xmlStr("leave")));
                    section->m_noteOffLeave = no == "true";
                }
            }
            DOMNode *chainNode = findNode(doc, (DOMElement*)sectionNode, "./chain");
            if (chainNode)
            {
                const char *ss[] = {"nextSection", "previousSection"};
                for (size_t i=0; i<2; i++)
                {
                    if (((DOMElement*)chainNode)->hasAttribute(m_xmlStr(ss[i])))
                    {
                        int s = 0;
                        std::string trackStr = xmlStdString(((DOMElement*)chainNode)->getAttribute(m_xmlStr(ss[i])));
                        if (trackStr == "last")
                            s = TrackDef::LastSection;
                        else
                        {
                            m_stringStream.clear();
                            m_stringStream << trackStr;
                            m_stringStream >> s;
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
                    if (((DOMElement*)chainNode)->hasAttribute(m_xmlStr(ts[i])))
                    {
                        int t = 0;
                        std::string trackStr = xmlStdString(((DOMElement*)chainNode)->getAttribute(m_xmlStr(ts[i])));
                        if (trackStr == "next")
                            t = TrackDef::NextTrack;
                        else if (trackStr == "current")
                            t = TrackDef::CurrentTrack;
                        else if (trackStr == "previous")
                            t = TrackDef::PreviousTrack;
                        else
                        {
                            m_stringStream.clear();
                            m_stringStream << trackStr;
                            m_stringStream >> t;
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
                    name = XMLString::transcode(((DOMElement*)partNode)->getAttribute(m_xmlStr("name")));
                    part = new SwPart(strdup(name));
                    XMLString::release(&name);
                }
                section->m_partList.push_back(part);
                int channel;
                xmlStream(((DOMElement*)partNode)->getAttribute(m_xmlStr("channel")))
                    >> channel;
                part->m_channel = channel-1;
                if (xmlStdString(((DOMElement*)partNode)->getAttribute(m_xmlStr("toggle"))) == "true")
                {
                    part->m_toggler.enable();
                }
                if (xmlStdString(((DOMElement*)partNode)->getAttribute(m_xmlStr("mono"))) == "true")
                {
                    part->m_mono = true;
                }
                if (((DOMElement*)partNode)->hasAttribute(m_xmlStr("sustainTranspose")))
                {
                    int tp = 0;
                    xmlStream(((DOMElement*)partNode)->getAttribute(m_xmlStr("sustainTranspose"))) >> tp;
                    part->m_transposer = new Transposer(tp);
                }
                DOMNode *controllerRemapNode = findNode(doc, (DOMElement*)partNode, "./controllerRemap");
                if (controllerRemapNode)
                {
                    delete part->m_controllerRemap; // default
                    std::string id = xmlStdString(((DOMElement*)controllerRemapNode)->getAttribute(m_xmlStr("id")));
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
                    if (((DOMElement*)rangeNode)->hasAttribute(m_xmlStr("lower")))
                    {
                        std::string note = xmlStdString(((DOMElement*)rangeNode)->getAttribute(m_xmlStr("lower")));
                        part->m_rangeLower = SwPart::stringToNoteNum(note.c_str());
                    }
                    if (((DOMElement*)rangeNode)->hasAttribute(m_xmlStr("upper")))
                    {
                        std::string note = xmlStdString(((DOMElement*)rangeNode)->getAttribute(m_xmlStr("upper")));
                        part->m_rangeUpper = SwPart::stringToNoteNum(note.c_str());
                    }
                }
                DOMNode *transposeNode = findNode(doc, (DOMElement*)partNode, "./transpose");
                if (transposeNode)
                {
                    if (((DOMElement*)transposeNode)->hasAttribute(m_xmlStr("offset")))
                    {
                        xmlStream(((DOMElement*)transposeNode)->getAttribute(m_xmlStr("offset")))
                            >> part->m_transpose;
                    }
                    DOMNode *customNode = findNode(doc, (DOMElement*)transposeNode, "./custom");
                    if (customNode)
                    {
                        part->m_customTransposeEnabled = true;
                        memset(part->m_customTranspose, 0, sizeof(part->m_customTranspose));
                        if (((DOMElement*)customNode)->hasAttribute(m_xmlStr("offset")))
                        {
                            xmlStream(((DOMElement*)customNode)->getAttribute(m_xmlStr("offset")))
                                >> part->m_customTransposeOffset;
                        }
                        DOMXPathResult *mapNodes = findNodes(doc, (DOMElement*)customNode, "./map");
                        for (size_t mapIdx = 0; mapNodes->snapshotItem(mapIdx); mapIdx++)
                        {
                            DOMNode *mapNode = mapNodes->getNodeValue();
                            int from;
                            xmlStream(((DOMElement*)mapNode)->getAttribute(m_xmlStr("from"))) >> from;
                            from = (from + 120) % 12;
                            int to;
                            xmlStream(((DOMElement*)mapNode)->getAttribute(m_xmlStr("to"))) >> to;
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
        char *name = XMLString::transcode(((DOMElement*)trackNode)->getAttribute(m_xmlStr("name")));
        setList.add(trackList, name);
        XMLString::release(&name);
    }
    trackNodes->release();
}

//! \brief Parse an XML file into a track list and a \a SetList.
int XMLParser::importTracks (const char *inFile, TrackList &tracks, SetList &setList)
{
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
    fixChain(tracks);
    return 0;
}

//! \brief Filters out whitespace-only text nodes during serialisation.
class WhiteSpaceFilter: public DOMLSSerializerFilter
{
public:
    virtual FilterAction acceptNode(const DOMNode *node) const
    {
        if (node->getNodeType() != DOMNode::TEXT_NODE)
            return DOMNodeFilter::FILTER_ACCEPT;
        const XMLCh *textPointer = node->getTextContent();
        bool isAllWhitespace = true;
        while (*textPointer && isAllWhitespace)
        {
            isAllWhitespace = isAllWhitespace && XMLChar1_0::isWhitespace(*textPointer);
            textPointer++;
        }
        if (isAllWhitespace)
            return DOMNodeFilter::FILTER_REJECT;
        return DOMNodeFilter::FILTER_ACCEPT;
    }
    virtual ShowType getWhatToShow() const
    {
        return DOMNodeFilter::SHOW_ALL;
    }
};

//! \brief Serialises a DOMDocument to a file.
void XMLParser::serialise(DOMDocument *doc, const char *outFile)
{
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(m_xmlStr("LS"));
    DOMLSSerializer* serializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    WhiteSpaceFilter wsf;
    serializer->setFilter(&wsf);
    serializer->getDomConfig()->setParameter(m_xmlStr("format-pretty-print"), true);
    DOMLSOutput *output = ((DOMImplementationLS*)impl)->createLSOutput();
    LocalFileFormatTarget target(outFile);
    output->setByteStream(&target);
    serializer->write(doc, output);
    delete output;
    delete serializer;
}

uint8_t XMLParser::getByteAttribute(DOMElement *node, const char *attr, int defaultByte)
{
    const XMLCh *c = node->getAttribute(m_xmlStr(attr));
    if (c == 0 || c[0] == 0)
    {
        if (defaultByte == -1)
        {
            Error e;
            e.stream() << "missing attribute: " << attr;
            throw(e);
        }
        else
            return (uint8_t)defaultByte;
    }
    return toByte(c);
}

void XMLParser::parsePerformances(DOMDocument *doc, std::vector<Fantom::Performance*> &performanceList)
{
    //std::cerr << "parse performances " << std::endl;
    performanceList.clear();
    DOMElement *root = doc->getDocumentElement();
    DOMNode *performanceListNode = findNode(doc, root, "/performances");
    DOMXPathResult *performanceNodes = findNodes(doc, (DOMElement*)performanceListNode, "./performance");
    for (size_t performanceIdx = 0; performanceNodes->snapshotItem(performanceIdx); performanceIdx++)
    {
        //std::cerr << "performance " << performanceIdx << std::endl;
        DOMNode *performanceNode = performanceNodes->getNodeValue();
        Fantom::Performance *performance = new Fantom::Performance;
        char *name = XMLString::transcode(((DOMElement*)performanceNode)->getAttribute(m_xmlStr("name")));
        for (size_t i=0; name[i] && i<(size_t)Fantom::NameLength; i++)
            performance->m_name[i] = name[i];
        XMLString::release(&name);
        DOMXPathResult *partNodes = findNodes(doc, (DOMElement*)performanceNode, "./part");
        for (size_t partIdx = 0; partNodes->snapshotItem(partIdx); partIdx++)
        {
            DOMNode *partNode = partNodes->getNodeValue();
            Fantom::Part *part = performance->m_partList + partIdx;
            part->m_channel = 1 + getByteAttribute((DOMElement*)partNode, "channel");
            part->m_bankSelectMsb = getByteAttribute((DOMElement*)partNode, "bankSelectMsb");
            part->m_bankSelectLsb = getByteAttribute((DOMElement*)partNode, "bankSelectLsb");
            part->m_programChange = getByteAttribute((DOMElement*)partNode, "programChange");
            bool b;
            part->constructPreset(b);
            part->m_volume = getByteAttribute((DOMElement*)partNode, "volume");
            part->m_transpose = getByteAttribute((DOMElement*)partNode, "transpose", 0);
            part->m_octave = getByteAttribute((DOMElement*)partNode, "octave", 0);
            part->m_keyRangeLower = getByteAttribute((DOMElement*)partNode, "keyRangeLower", 0);
            part->m_keyRangeUpper = getByteAttribute((DOMElement*)partNode, "keyRangeUpper", 127);
            part->m_fadeWidthLower = getByteAttribute((DOMElement*)partNode, "fadeWidthLower", 0);
            part->m_fadeWidthUpper = getByteAttribute((DOMElement*)partNode, "fadeWidthUpper", 0);
            DOMNode *patchNode = findNode(doc, (DOMElement*)partNode, "./patch");
            name = XMLString::transcode(((DOMElement*)patchNode)->getAttribute(m_xmlStr("name")));
            for (size_t i=0; name[i] && i<(size_t)Fantom::NameLength; i++)
                part->m_patch.m_name[i] = name[i];
            XMLString::release(&name);
        }
        partNodes->release();

        performanceList.push_back(performance);
    }
    performanceNodes->release();
}

int XMLParser::importPerformances(const char *inFile, std::vector<Fantom::Performance*> &performanceList)
{
    XercesDOMParser* parser = new XercesDOMParser();
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    //parser->setDoNamespaces(true);

    ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
    parser->setErrorHandler(errHandler);

    try {
        parser->parse(inFile);
        DOMDocument *doc = parser->getDocument();
        parsePerformances(doc, performanceList);
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
    return 0;
}


//! \brief Serialises Fantom Performances to an XML file.
int XMLParser::exportPerformances(const char *outFile, const Fantom::Performance *performanceList,
            uint32_t nofPerformances)
{
    XMLCh stringBuf[100];
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(m_xmlStr("core"));
    DOMDocument *doc = impl->createDocument(0, m_xmlStr("performances"), 0);
    DOMElement *root = doc->getDocumentElement();
    root->setAttribute(m_xmlStr("version"), m_xmlStr("1"));
    for (int performanceIdx = 0; performanceIdx < (int)nofPerformances; performanceIdx++)
    {
        const Fantom::Performance *performance = performanceList + performanceIdx;
        DOMElement *performanceNode = doc->createElement(m_xmlStr("performance"));
        root->appendChild(performanceNode);
        std::cerr << "performance " << performanceIdx << ": " << performance->m_name << std::endl;
        performanceNode->setAttribute(m_xmlStr("name"), m_xmlStr(performance->m_name));
        for (int partIdx = 0; partIdx < Fantom::Performance::NofParts; partIdx++)
        {
            if (performanceIdx ==1 && partIdx == 14)
            {
                static int qqq;
                qqq++;
            }
            //std::cerr << "part " << partIdx << std::endl;
            const Fantom::Part *part = performance->m_partList+partIdx;
            DOMElement *partNode = doc->createElement(m_xmlStr("part"));
            performanceNode->appendChild(partNode);
            XMLString::binToText(part->m_channel+1, stringBuf, 99, 10);
            partNode->setAttribute(m_xmlStr("channel"), stringBuf);
            XMLString::binToText(part->m_bankSelectMsb, stringBuf, 99, 10);
            partNode->setAttribute(m_xmlStr("bankSelectMsb"), stringBuf);
            XMLString::binToText(part->m_bankSelectLsb, stringBuf, 99, 10);
            partNode->setAttribute(m_xmlStr("bankSelectLsb"), stringBuf);
            XMLString::binToText(part->m_programChange, stringBuf, 99, 10);
            partNode->setAttribute(m_xmlStr("programChange"), stringBuf);
            partNode->setAttribute(m_xmlStr("preset"), m_xmlStr(part->m_preset));
            XMLString::binToText(part->m_volume, stringBuf, 99, 10);
            partNode->setAttribute(m_xmlStr("volume"), stringBuf);
            if (part->m_transpose != 0)
            {
                XMLString::binToText(part->m_transpose, stringBuf, 99, 10);
                partNode->setAttribute(m_xmlStr("transpose"), stringBuf);
            }
            if (part->m_octave != 0)
            {
                XMLString::binToText(part->m_octave, stringBuf, 99, 10);
                partNode->setAttribute(m_xmlStr("octave"), stringBuf);
            }
            if (part->m_keyRangeLower != Midi::Note::C0)
            {
                XMLCh *note = XMLString::transcode(Midi::noteName(part->m_keyRangeLower));
                partNode->setAttribute(m_xmlStr("keyRangeLower"), note);
                XMLString::release(&note);
            }
            if (part->m_keyRangeUpper != Midi::Note::G10)
            {
                XMLCh *note = XMLString::transcode(Midi::noteName(part->m_keyRangeUpper));
                partNode->setAttribute(m_xmlStr("keyRangeUpper"), note);
                XMLString::release(&note);
            }
            if (part->m_fadeWidthLower != 0)
            {
                XMLString::binToText(part->m_fadeWidthLower, stringBuf, 99, 10);
                partNode->setAttribute(m_xmlStr("fadeWidthLower"), stringBuf);
            }
            if (part->m_fadeWidthUpper != 0)
            {
                XMLString::binToText(part->m_fadeWidthUpper, stringBuf, 99, 10);
                partNode->setAttribute(m_xmlStr("fadeWidthUpper"), stringBuf);
            }
            const Fantom::Patch *patch = &part->m_patch;
            DOMElement *patchNode = doc->createElement(m_xmlStr("patch"));
            partNode->appendChild(patchNode);
            patchNode->setAttribute(m_xmlStr("name"), m_xmlStr(patch->m_name));
        } // foreach part
    } // foreach Performance
    serialise(doc, outFile);
    doc->release();
    std::cerr << "done" << std::endl;
    return 0;
}

#ifdef EXPORT_TRACKS
// Exports a track list to an XML file
void exportTracks(const std::vector<Track*> &tracks, const char *filename)
{
    XMLCh stringBuf[100];
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(m_xmlStr("core"));
    DOMDocument *doc = impl->createDocument(0, m_xmlStr("tracks"), 0);
    DOMElement *root = doc->getDocumentElement();
    root->setAttribute(m_xmlStr("version"), m_xmlStr("1"));
    int trackIdx = 0;
    for (std::vector<Track*>::const_iterator trackIt = tracks.begin(); trackIt != tracks.end(); trackIt++)
    {
        const Track *track = *trackIt;
        DOMElement *trackNode = doc->createElement(m_xmlStr("track"));
        root->appendChild(trackNode);
        trackNode->setAttribute(m_xmlStr("name"), m_xmlStr(track->m_name));
        if (track->m_startSection != 0)
        {
            XMLString::binToText(track->m_startSection, stringBuf, 99, 10);
            trackNode->setAttribute(m_xmlStr("startSection"), stringBuf);
        }
        if (track->m_chain != 0)
        {
            trackNode->setAttribute(m_xmlStr("chain"), toBool(true));
        }
        int sectionIdx = 0;
        for (std::vector<Section*>::const_iterator sectionIt = track->m_section.begin(); sectionIt != track->m_section.end(); sectionIt++)
        {
            const Section *section = *sectionIt;
            DOMElement *sectionNode = doc->createElement(m_xmlStr("section"));
            trackNode->appendChild(sectionNode);
            sectionNode->setAttribute(m_xmlStr("name"), m_xmlStr(section->m_name));
            if (!section->m_noteOffEnter || !section->m_noteOffLeave)
            {
                DOMElement *n = doc->createElement(m_xmlStr("noteOff"));
                n->setAttribute(m_xmlStr("enter"), toBool(section->m_noteOffEnter));
                n->setAttribute(m_xmlStr("leave"), toBool(section->m_noteOffLeave));
                sectionNode->appendChild(n);
            }
            if (track->m_chain && (section->m_nextTrack != -1 || section->m_nextSection != -1 || section->m_previousTrack != -1 || section->m_previousSection != -1))
            {
                DOMElement *n = doc->createElement(m_xmlStr("chain"));
                addAttribute(n, "previousTrack", section->m_previousTrack, -1, trackIdx);
                addAttribute(n, "previousSection", section->m_previousSection);
                addAttribute(n, "nextTrack", section->m_nextTrack, -1, trackIdx);
                addAttribute(n, "nextSection", section->m_nextSection);
                sectionNode->appendChild(n);
            }
            int partIdx = 0;
            for (std::vector<SwPart*>::const_iterator partIt = section->m_part.begin(); partIt != section->m_part.end(); partIt++)
            {
                const SwPart *part = *partIt;
                DOMElement *partNode = doc->createElement(m_xmlStr("part"));
                sectionNode->appendChild(partNode);
                if (part->m_name)
                    partNode->setAttribute(m_xmlStr("name"), m_xmlStr(part->m_name));
                addAttribute(partNode, "channel", 1+part->m_channel);
                if (part->m_toggler.enabled())
                {
                    partNode->setAttribute(m_xmlStr("toggle"), m_xmlStr("true"));
                }
                if (part->m_mono)
                {
                    partNode->setAttribute(m_xmlStr("mono"), m_xmlStr("true"));
                }
                if (part->m_transposer)
                {
                    addAttribute(partNode, "sustainTranspose", part->m_transposer->m_transpose, 1000, 1000);
                }
                if (strcmp(part->m_controllerRemap->name(), "default") != 0)
                {
                    DOMElement *remapNode = doc->createElement(m_xmlStr("controllerRemap"));
                    partNode->appendChild(remapNode);
                    remapNode->setAttribute(m_xmlStr("id"), m_xmlStr(part->m_controllerRemap->name()));
                }
                DOMElement *rangeNode = doc->createElement(m_xmlStr("range"));
                partNode->appendChild(rangeNode);
                XMLCh *c = XMLString::transcode(Midi::noteName(part->m_rangeLower));
                rangeNode->setAttribute(m_xmlStr("lower"), c);
                XMLString::release(&c);
                c = XMLString::transcode(Midi::noteName(part->m_rangeUpper));
                rangeNode->setAttribute(m_xmlStr("upper"), c);
                XMLString::release(&c);

                DOMElement *tpNode = doc->createElement(m_xmlStr("transpose"));
                partNode->appendChild(tpNode);
                if (part->m_customTransposeEnabled)
                {
                    addAttribute(tpNode, "offset", part->m_transpose);
                    DOMElement *customNode = doc->createElement(m_xmlStr("custom"));
                    tpNode->appendChild(customNode);
                    addAttribute(customNode, "offset", part->m_customTransposeOffset);
                    for (int i=0; i<12; i++)
                    {
                        DOMElement *mapNode = doc->createElement(m_xmlStr("map"));
                        customNode->appendChild(mapNode);
                        addAttribute(mapNode, "from", i);
                        addAttribute(mapNode, "to", part->m_customTranspose[i],-100, -100);
                    }
                }
                else
                {
                    addAttribute(tpNode, "offset", part->m_transpose);
                }
                partIdx++;
            } // foreach part
            sectionIdx++;
        } // foreach section
        trackIdx++;
    } // foreach track
    serialise(doc, filename);
    doc->release();
}
#endif // EXPORT_TRACKS

XML::XML()
{
    m_xmlParser = new XMLParser;
}

XML::~XML()
{
    delete m_xmlParser;
}

int XML::importTracks (const char *inFile, TrackList &tracks, SetList &setList)
{
    return m_xmlParser->importTracks(inFile, tracks, setList);
}

int XML::exportPerformances(const char *outFile, const Fantom::Performance *performanceList,
            uint32_t nofPerformances)
{
    return m_xmlParser->exportPerformances(outFile, performanceList, nofPerformances);
}

int XML::importPerformances(const char *inFile, std::vector<Fantom::Performance*> &performanceList)
{
    return m_xmlParser->importPerformances(inFile, performanceList);
}
