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

// Adds float values to a node as text children
void addFloatsToNode(DOMDocument *doc, DOMNode *node, XMLStringTokenizer *floatTokens, const char **leafNames)
{
    for (size_t i=0; leafNames[i]; i++)
    {
        DOMElement *floatValueNode = doc->createElement(xmlStr(leafNames[i]));
        DOMText *floatText = doc->createTextNode(floatTokens->nextToken());
        floatValueNode->appendChild(floatText);
        node->appendChild(floatValueNode);
    }
}

// Re-organises the text value of a node (which consists of a list of floats) into
// properly tagged child nodes
void untangleFloatsInNode(DOMDocument *doc, DOMNode *node, const char *by, const ConstXmlStringList &byLabel, const char **leafNames)
{
    DOMNode *parent = node->getParentNode();
    DOMNode *newNode = node;
    XMLStringTokenizer *floatTokens = new XMLStringTokenizer(node->getTextContent());
    node->setTextContent(0);
    while (node->hasChildNodes())
    {
        node->removeChild(node->getFirstChild());
    }
    size_t idx = 0;
    for (ConstXmlStringList::const_iterator label = byLabel.begin(); label != byLabel.end(); label++, idx++)
    {
        if (idx > 0) // first node is re-used, other nodes are created
        {
            newNode = doc->createElement(node->getNodeName());
            parent->insertBefore(newNode, node);
        }
        ((DOMElement*)newNode)->setAttribute(xmlStr(by), *label);
        addFloatsToNode(doc, newNode, floatTokens, leafNames);
    }
    delete floatTokens;
}

// Converts a DOMDocument from mvnx version 3 to version 4
void fixMvnx(DOMDocument *doc)
{
    DOMElement *root = doc->getDocumentElement();
    DOMNode *mvnx = findNode(doc, root, "/mvnx");
    ((DOMElement*)mvnx)->setAttribute(xmlStr("version"), xmlStr("4"));
    DOMXPathResult *subjects = findNodes(doc, (DOMElement*)mvnx, "./subject");
    for (size_t subjectIdx = 0; subjects->snapshotItem(subjectIdx); subjectIdx++)
    {
        DOMNode *subject = subjects->getNodeValue();
        DOMXPathResult *segments = findNodes(doc, (DOMElement*)subject, "./segments/segment");
        ConstXmlStringList segmentLabels;
        for (size_t segmentIdx = 0; segments->snapshotItem(segmentIdx); segmentIdx++)
        {
            DOMNode *segment = segments->getNodeValue();
            const XMLCh *label = ((DOMElement*)segment)->getAttribute(xmlStr("label"));
            segmentLabels.push_back(label);
            DOMXPathResult *points = findNodes(doc, (DOMElement*)segment, "./points/point");
            DOMNode *pointsRemove = findNode(doc, (DOMElement*)segment, "./points");
            ((DOMElement*)segment)->removeChild(pointsRemove);
            for (size_t pointIdx = 0; points->snapshotItem(pointIdx); pointIdx++)
            {
                DOMNode *point = points->getNodeValue();
                XMLStringTokenizer *floatTokens = new XMLStringTokenizer(point->getTextContent());
                point->setTextContent(0);
                const char *leafName[] = {"x", "y", "z", 0};
                addFloatsToNode(doc, point, floatTokens, leafName);
                segment->appendChild(point);
                delete floatTokens;
            }
            delete points;
        }
        delete segments;
        ConstXmlStringList jointLabels;
        DOMXPathResult *joints = findNodes(doc, (DOMElement*)subject, "./joints/joint");
        size_t strLenConnector = strlen("connector");
        for (size_t jointIdx = 0; joints->snapshotItem(jointIdx); jointIdx++)
        {
            DOMNode *joint = joints->getNodeValue();
            const XMLCh *label = ((DOMElement*)joint)->getAttribute(xmlStr("label"));
            jointLabels.push_back(label);
            DOMNodeList *connectorList = joint->getChildNodes();
            NodeList newConnectorList;
            for (size_t connectorIdx=0; connectorIdx<connectorList->getLength(); connectorIdx++)
            {
                DOMNode *connector = connectorList->item(connectorIdx);
                const XMLCh *cn = connector->getNodeName();
                if (XMLString::compareNString(cn, xmlStr("connector"), strLenConnector) == 0)
                {
                    const XMLCh *id = cn + strLenConnector;
                    XMLCh *segmentLabel = 0, *pointLabel = 0;
                    DOMNodeList *childList = connector->getChildNodes();
                    for (size_t i=0; i<childList->getLength(); i++)
                    {
                        DOMNode *node = childList->item(i);
                        if (node->getNodeType() == DOMNode::TEXT_NODE)
                        {
                            XMLStringTokenizer tok(node->getTextContent(), xmlStr(" \r\n\t/"));
                            while (tok.hasMoreTokens())
                            {
                                if (!segmentLabel)
                                    segmentLabel = tok.nextToken();
                                else
                                    pointLabel = tok.nextToken();
                            }
                            if (pointLabel)
                            {
                                DOMElement *newConnector = doc->createElement(xmlStr("connector"));
                                newConnector->setAttribute(xmlStr("id"), id);
                                newConnector->setAttribute(xmlStr("segment"), segmentLabel);
                                newConnector->setAttribute(xmlStr("point"), pointLabel);
                                newConnectorList.push_back(newConnector);
                                break;
                            }
                        }
                    }
                }
            } // foreach connector
            while (joint->hasChildNodes())
            {
                joint->removeChild(joint->getFirstChild());
            }
            for (NodeList::iterator i = newConnectorList.begin(); i != newConnectorList.end(); i++)
            {
                joint->appendChild(*i);
            }
        } // foreach joint
        delete joints;
        DOMXPathResult *frames = findNodes(doc, (DOMElement*)subject, "./frames/frame");
        for (size_t frameIdx = 0; frames->snapshotItem(frameIdx); frameIdx++)
        {
            DOMNode *frame = frames->getNodeValue();
#if 0
            if (frameIdx % 10 == 0)
            {
                double percent = 100.0 * frameIdx / frames->getSnapshotLength();
                std::cout << "fixing " << percent << "%\r";
                std::cout.flush();
            }
#endif
            DOMNode *com = findNode(doc, (DOMElement*)frame, "./centerOfMass");
            if (com)
            {
                XMLStringTokenizer *floatTokens = new XMLStringTokenizer(com->getTextContent());
                com->setTextContent(0);
                const char *leafName[] = {"x", "y", "z", 0};
                addFloatsToNode(doc, com, floatTokens, leafName);
                delete floatTokens;
            }
            struct {
                const char *m_name;
                const char *m_by;
                const ConstXmlStringList *m_byLabel;
                const char *m_leafNames[5];
            } untangleNodes[] =
            {
                { "./orientation",          "segment", &segmentLabels, {"w", "x", "y", "z", 0 } },
                { "./position",             "segment", &segmentLabels, {"x", "y", "z", 0 } },
                { "./velocity",             "segment", &segmentLabels, {"x", "y", "z", 0 } },
                { "./acceleration",         "segment", &segmentLabels, {"x", "y", "z", 0 } },
                { "./angularVelocity",      "segment", &segmentLabels, {"x", "y", "z", 0 } },
                { "./angularAcceleration",  "segment", &segmentLabels, {"x", "y", "z", 0 } },
                { "./jointAngle",       "joint",   &jointLabels,   {"angle", "angle", "angle", 0 } },
                { "./jointAngleXZY",    "joint",   &jointLabels,   {"angle", "angle", "angle", 0 } },
                { 0, 0, 0, { 0 } }
            }, *u;
            for (u = &untangleNodes[0]; u->m_name; u++)
            {
                DOMNode *node = findNode(doc, (DOMElement*)frame, u->m_name);
                if (node)
                {
                    untangleFloatsInNode(doc, node, u->m_by, *u->m_byLabel, u->m_leafNames);
                }
            }
        } // foreach frame
        delete frames;
    } // foreach subject
    delete subjects;
}

void parseTracks(DOMDocument *doc, std::vector<Track*> &trackList)
{
    DOMElement *root = doc->getDocumentElement();
    DOMNode *tracksNode = findNode(doc, root, "/tracks");
    DOMXPathResult *trackNodes = findNodes(doc, (DOMElement*)tracksNode, "./track");
    for (size_t trackIdx = 0; trackNodes->snapshotItem(trackIdx); trackIdx++)
    {
        DOMNode *trackNode = trackNodes->getNodeValue();
        const XMLCh *name = ((DOMElement*)trackNode)->getAttribute(xmlStr("name"));
        Track *track = new Track(XMLString::transcode(name));
        const XMLCh *sss = ((DOMElement*)trackNode)->getAttribute(xmlStr("startSection"));
        unsigned int ssi;
        if (XMLString::textToBin(sss, ssi))
        {
            track->m_startSection = ssi;
        }
        const XMLCh *chs = ((DOMElement*)trackNode)->getAttribute(xmlStr("chain"));
        if (0 == XMLString::compareIString(chs, xmlStr("true")))
        {
            track->m_chain = true;
        }
        DOMXPathResult *sectionNodes = findNodes(doc, (DOMElement*)trackNode, "./section");
        for (size_t sectionIdx = 0; sectionNodes->snapshotItem(sectionIdx); sectionIdx++)
        {
            DOMNode *sectionNode = sectionNodes->getNodeValue();
            const XMLCh *name = ((DOMElement*)sectionNode)->getAttribute(xmlStr("name"));
            Section *section = new Section(XMLString::transcode(name));
            DOMXPathResult *partNodes = findNodes(doc, (DOMElement*)trackNode, "./part");
            for (size_t partIdx = 0; partNodes->snapshotItem(partIdx); partIdx++)
            {
                DOMNode *partNode = partNodes->getNodeValue();
                const XMLCh *name = ((DOMElement*)partNode)->getAttribute(xmlStr("name"));
                SwPart *part = new SwPart(XMLString::transcode(name));
                section->m_part.push_back(part);
            }
            track->m_section.push_back(section);
        }
        trackList.push_back(track);
    }
}

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
    //xmlStr.clear();
    //xPath.clear();
    XMLPlatformUtils::Terminate();
    return 0;
}

XMLCh *toBool(bool b)
{
    if (b)
        return xmlStr("true");
    else
        return xmlStr("false");
}

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
                    partNode->setAttribute(xmlStr("sustainTranspose"), xmlStr("true"));
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
                if (part->m_transpose >= 500)
                {
                    addAttribute(tpNode, "offset", part->m_transpose - 1000);
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
