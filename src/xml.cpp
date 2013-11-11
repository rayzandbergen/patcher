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

XERCES_CPP_NAMESPACE_USE

typedef std::vector<DOMNode*> NodeList;
typedef std::list<const XMLCh*> ConstXmlStringList;

// This class template caches T objects that are created from string constants.
// It is used to create cache classes for XMLCh strings and DOMXPathExpressions.
template <class T> class XMLCache
{
    std::map<const char *, T *> m_map;
protected:
    virtual T *create(const char *stringConstant, DOMDocument *doc = 0) = 0;
    virtual void release(T *) = 0;
public:
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
    void clear()
    {
        for (typename std::map<const char *, T *>::iterator i = m_map.begin(); i != m_map.end(); i++)
        {
            release(i->second);
        }
    }
};

// This class caches XMLCh strings to avoid repeated conversions
// of the same string constants.
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

static XMLStringCache xmlStr;

// This class caches DOMXPathExpression instances to avoid repeated constructions
// from the same staring constants.
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

static XPathCache xPath;

// This class filters out whitespace-only text nodes
// during serialisation.
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

// Serialises a DOMDocument to a file
void serialise(DOMDocument *doc, const char *outFile)
{
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(xmlStr("LS"));
    DOMLSSerializer* serializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    WhiteSpaceFilter wsf;
    serializer->setFilter(&wsf);
    serializer->getDomConfig()->setParameter(xmlStr("format-pretty-print"), true);
    DOMLSOutput *output = ((DOMImplementationLS*)impl)->createLSOutput();
    LocalFileFormatTarget target(outFile);
    output->setByteStream(&target);
    serializer->write(doc, output);
    delete output;
    delete serializer;
}

// Finds the first node that matches an XPath expression
DOMNode *findNode(DOMDocument *doc, const DOMElement *node, const char *path)
{
    DOMXPathResult *r = xPath(path, doc)->evaluate(node, DOMXPathResult::FIRST_ORDERED_NODE_TYPE, 0);
    DOMNode *rv = r->getNodeValue();
    delete r;
    return rv;
}

// Finds nodes that match an XPath expression
DOMXPathResult *findNodes(DOMDocument *doc, const DOMElement *node, const char *path)
{
    return xPath(path, doc)->evaluate(node, DOMXPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0);
}

std::stringstream stringStream;

// Puts an XMLString into an std::stream
std::stringstream &xmlStream(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    stringStream.clear();
    stringStream << s;
    XMLString::release(&s);
    return stringStream;
}

// Puts an XMLString into an std::string
std::string xmlStdString(const XMLCh *c)
{
    char *s = XMLString::transcode(c);
    std::string stdString(s);
    XMLString::release(&s);
    return stdString;
}

// Parses a DOM document into a track list
void parseTracks(DOMDocument *doc, std::vector<Track*> &trackList)
{
    DOMElement *root = doc->getDocumentElement();
    DOMNode *tracksNode = findNode(doc, root, "/tracks");
    DOMXPathResult *trackNodes = findNodes(doc, (DOMElement*)tracksNode, "./track");
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
                if (((DOMElement*)chainNode)->hasAttribute(xmlStr("nextSection")))
                {
                    xmlStream(((DOMElement*)chainNode)->getAttribute(xmlStr("nextSection")))
                        >> section->m_nextSection;
                }
                if (((DOMElement*)chainNode)->hasAttribute(xmlStr("previousSection")))
                {
                    xmlStream(((DOMElement*)chainNode)->getAttribute(xmlStr("previousSection")))
                        >> section->m_previousSection;
                }
                const char *ts[] = {"nextTrack", "previousTrack"};
                for (size_t i=0; i<2; i++)
                {
                    if (((DOMElement*)chainNode)->hasAttribute(xmlStr(ts[i])))
                    {
                        int t = 0;
                        std::string trackStr = xmlStdString(((DOMElement*)chainNode)->getAttribute(xmlStr(ts[i])));
                        if (trackStr == "next")
                            t = trackIdx + 1;
                        else if (trackStr == "current")
                            t = trackIdx;
                        else if (trackStr == "previous")
                            t = trackIdx - 1;
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
                section->m_part.push_back(part);
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
                        part->m_controllerRemap = new ControllerRemapVolQuadratic;
                    else if (id == "volReverse")
                        part->m_controllerRemap = new ControllerRemapVolReverse;
                    else
                    {
                        // huh?
                    }
                }
                if (!part->m_controllerRemap)
                    part->m_controllerRemap = new ControllerRemap;

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
            track->m_section.push_back(section);
            partNodes->release();
        }
        trackList.push_back(track);
        sectionNodes->release();
    }
    trackNodes->release();
}

// Parses an XML file into a track list
int importTracks (const char *inFile, std::vector<Track*> &tracks)
{
    try {
        XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cout << "Error during initialization! :\n"
                  << message << "\n";
        XMLString::release(&message);
        return 1;
    }

    XercesDOMParser* parser = new XercesDOMParser();
    parser->setValidationScheme(XercesDOMParser::Val_Always);
    //parser->setDoNamespaces(true);

    ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
    parser->setErrorHandler(errHandler);

    try {
        parser->parse(inFile);
        DOMDocument *doc = parser->getDocument();
        parseTracks(doc, tracks);
    }
    catch (const XMLException& toCatch) {
        char* message = XMLString::transcode(toCatch.getMessage());
        std::cout << "Exception message is: \n"
                  << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (const DOMException& toCatch) {
        char* message = XMLString::transcode(toCatch.msg);
        std::cout << "Exception message is: \n"
                  << message << "\n";
        XMLString::release(&message);
        return -1;
    }
    catch (...) {
        std::cout << "Unexpected Exception \n" ;
        return -1;
    }

    delete parser;
    delete errHandler;
#ifndef EXPORT_TRACKS
    xmlStr.clear();
    xPath.clear();
#endif
    XMLPlatformUtils::Terminate();
    return 0;
}

// Converts a boolean to an XMLString
XMLCh *toBool(bool b)
{
    if (b)
        return xmlStr("true");
    else
        return xmlStr("false");
}

// Adds an attribute to a DOM element
void addAttribute(DOMElement *n, const char *attrName, int x, int def = -1, int ref = -3)
{
    if (x != def)
    {
        const char *s = 0;
        if (x == ref)
            s = "current";
        else if (x == ref-1)
            s = "previous";
        else if (x == ref+1)
            s = "next";
        if (s)
        {
            n->setAttribute(xmlStr(attrName), xmlStr(s));
        }
        else
        {
            XMLCh stringBuf[100];
            XMLString::binToText(x, stringBuf, 99, 10);
            n->setAttribute(xmlStr(attrName), stringBuf);
        }
    }
}

#ifdef EXPORT_TRACKS
// Exports a track list to an XML file
void exportTracks(const std::vector<Track*> &tracks, const char *filename)
{
    XMLCh stringBuf[100];
    XMLPlatformUtils::Initialize();
    DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(xmlStr("core"));
    DOMDocument *doc = impl->createDocument(0, xmlStr("tracks"), 0);
    DOMElement *root = doc->getDocumentElement();
    root->setAttribute(xmlStr("version"), xmlStr("1"));
    int trackIdx = 0;
    for (std::vector<Track*>::const_iterator trackIt = tracks.begin(); trackIt != tracks.end(); trackIt++)
    {
        const Track *track = *trackIt;
        DOMElement *trackNode = doc->createElement(xmlStr("track"));
        root->appendChild(trackNode);
        trackNode->setAttribute(xmlStr("name"), xmlStr(track->m_name));
        if (track->m_startSection != 0)
        {
            XMLString::binToText(track->m_startSection, stringBuf, 99, 10);
            trackNode->setAttribute(xmlStr("startSection"), stringBuf);
        }
        if (track->m_chain != 0)
        {
            trackNode->setAttribute(xmlStr("chain"), toBool(true));
        }
        int sectionIdx = 0;
        for (std::vector<Section*>::const_iterator sectionIt = track->m_section.begin(); sectionIt != track->m_section.end(); sectionIt++)
        {
            const Section *section = *sectionIt;
            DOMElement *sectionNode = doc->createElement(xmlStr("section"));
            trackNode->appendChild(sectionNode);
            sectionNode->setAttribute(xmlStr("name"), xmlStr(section->m_name));
            if (!section->m_noteOffEnter || !section->m_noteOffLeave)
            {
                DOMElement *n = doc->createElement(xmlStr("noteOff"));
                n->setAttribute(xmlStr("enter"), toBool(section->m_noteOffEnter));
                n->setAttribute(xmlStr("leave"), toBool(section->m_noteOffLeave));
                sectionNode->appendChild(n);
            }
            if (track->m_chain && (section->m_nextTrack != -1 || section->m_nextSection != -1 || section->m_previousTrack != -1 || section->m_previousSection != -1))
            {
                DOMElement *n = doc->createElement(xmlStr("chain"));
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
                DOMElement *partNode = doc->createElement(xmlStr("part"));
                sectionNode->appendChild(partNode);
                if (part->m_name)
                    partNode->setAttribute(xmlStr("name"), xmlStr(part->m_name));
                addAttribute(partNode, "channel", 1+part->m_channel);
                if (part->m_toggler.enabled())
                {
                    partNode->setAttribute(xmlStr("toggle"), xmlStr("true"));
                }
                if (part->m_mono)
                {
                    partNode->setAttribute(xmlStr("mono"), xmlStr("true"));
                }
                if (part->m_transposer)
                {
                    addAttribute(partNode, "sustainTranspose", part->m_transposer->m_transpose, 1000, 1000);
                }
                if (strcmp(part->m_controllerRemap->name(), "default") != 0)
                {
                    DOMElement *remapNode = doc->createElement(xmlStr("controllerRemap"));
                    partNode->appendChild(remapNode);
                    remapNode->setAttribute(xmlStr("id"), xmlStr(part->m_controllerRemap->name()));
                }
                DOMElement *rangeNode = doc->createElement(xmlStr("range"));
                partNode->appendChild(rangeNode);
                XMLCh *c = XMLString::transcode(Midi::noteName(part->m_rangeLower));
                rangeNode->setAttribute(xmlStr("lower"), c);
                XMLString::release(&c);
                c = XMLString::transcode(Midi::noteName(part->m_rangeUpper));
                rangeNode->setAttribute(xmlStr("upper"), c);
                XMLString::release(&c);

                DOMElement *tpNode = doc->createElement(xmlStr("transpose"));
                partNode->appendChild(tpNode);
                if (part->m_customTransposeEnabled)
                {
                    addAttribute(tpNode, "offset", part->m_transpose);
                    DOMElement *customNode = doc->createElement(xmlStr("custom"));
                    tpNode->appendChild(customNode);
                    addAttribute(customNode, "offset", part->m_customTransposeOffset);
                    for (int i=0; i<12; i++)
                    {
                        DOMElement *mapNode = doc->createElement(xmlStr("map"));
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
    XMLPlatformUtils::Terminate();
}
#endif // EXPORT_TRACKS

void clearTracks(std::vector<Track*> &trackList)
{
    for (std::vector<Track *>::iterator i = trackList.begin(); i != trackList.end(); i++)
        delete *i;
}
