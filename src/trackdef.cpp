#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "trackdef.h"
#include "fantom.h"
#include "error.h"

bool Section::next(int *nextTrack, int *nextSection)
{
    if (m_nextTrack >= 0 && m_nextSection >= 0)
    {
        *nextTrack = m_nextTrack;
        *nextSection = m_nextSection;
        return true;
    }
    return false;
}

bool Section::previous(int *nextTrack, int *nextSection)
{
    if (m_previousTrack >= 0 && m_previousSection >= 0)
    {
        *nextTrack = m_previousTrack;
        *nextSection = m_previousSection;
        return true;
    }
    return false;
}

Section::Section(const char *name, bool noteOffEnter, bool noteOffLeave):
        m_name(name), m_noteOffEnter(noteOffEnter), m_noteOffLeave(noteOffLeave),
        m_nextTrack(-1), m_nextSection(-1),
        m_previousTrack(-1), m_previousSection(-1)
{
}

Section::~Section()
{
    clear();
}

void Section::clear()
{
    for (std::vector<SwPart *>::iterator i = m_part.begin(); i != m_part.end(); i++)
        delete *i;
    m_part.clear();
    free((void*)m_name);
    m_name = 0;
}

SwPart::SwPart(const char *name, uint8_t channel, int transpose,
            const char *lower, const char *upper,
            bool mono, Transposer *t):
        m_name(name), m_channel(channel), m_transpose(transpose),
        m_mono(mono), m_transposer(t)
{
    m_rangeLower = lower ? stringToNoteNum(lower) : 0;
    m_rangeUpper = upper ? stringToNoteNum(upper): 127;
    m_controllerRemap = new ControllerRemap;
}

SwPart::~SwPart()
{
    clear();
}

void SwPart::clear()
{
    delete m_controllerRemap;
    m_controllerRemap = 0;
    delete m_transposer;
    m_transposer = 0;
    free((void*)m_name);
    m_name = 0;
}

uint8_t SwPart::stringToNoteNum(const char *s)
{
    const struct NoteOffset {
        char m_char;
        uint8_t m_offset;
    } noteOffset[] = {
        {'C', 0 },
        {'D', 2 },
        {'E', 4 },
        {'F', 5 },
        {'G', 7 },
        {'A', 9 },
        {'B', 11 }
    };

    uint8_t num = 255;
    int c = toupper(s[0]);
    for (uint8_t i=0; i<7; i++)
        if (c == noteOffset[i].m_char)
        {
            num = noteOffset[i].m_offset;
            break;
        }
    if (num == 255)
        throw(Error("SwPart::stringToNoteNum internal error"));
    uint8_t i = 1;
    if (s[i] == '#')
    {
        num++;
        i++;
    }
    else if (s[i] == 'b')
    {
        num--;
        i++;
    }
    uint8_t oct = s[i++] - '0';
    if (s[i])
    {
        oct = 10*oct + (s[i] - '0');
    }
    num += (uint8_t)12 * oct;
    return num;
}

Track::Track(const char *name, bool chain, int startSection): m_name(name), m_chain(chain), m_startSection(startSection)
{
}

Track::~Track()
{
    clear();
}

void Track::clear()
{
    for (std::vector<Section *>::iterator i = m_section.begin(); i != m_section.end(); i++)
        delete *i;
    m_section.clear();
    free((void*)m_name);
    m_name = 0;
}

void Track::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name);
    screen->printLog("%s.chain:%d\n", prefix, m_chain);
    screen->printLog("%s.startSection:%d\n", prefix, m_startSection);
    screen->printLog("%s.nofSections:%d\n", prefix, nofSections());
    char nestedPrefix[100];
    for (int i=0; i<nofSections(); i++)
    {
        sprintf(nestedPrefix, "%s.section%02d", prefix, 1+i);
        m_section[i]->dumpToLog(screen, nestedPrefix);
    }
}

void Section::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name);
    screen->printLog("%s.noteOffEnter:%d\n", prefix, m_noteOffEnter);
    screen->printLog("%s.noteOffLeave:%d\n", prefix, m_noteOffLeave);
    screen->printLog("%s.nextTrack:%d\n", prefix, m_nextTrack);
    screen->printLog("%s.nextSection:%d\n", prefix, m_nextSection);
    screen->printLog("%s.previousTrack:%d\n", prefix, m_previousTrack);
    screen->printLog("%s.previousSection:%d\n", prefix, m_previousSection);
    screen->printLog("%s.nofSwParts:%d\n", prefix, nofParts());
    char nestedPrefix[100];
    for (int i=0; i<nofParts(); i++)
    {
        sprintf(nestedPrefix, "%s.swPart%02d", prefix, 1+i);
        m_part[i]->dumpToLog(screen, nestedPrefix);
    }
}

void SwPart::dumpToLog(Screen *screen, const char *prefix) const
{
    screen->printLog("%s.name:%s\n", prefix, m_name ? m_name : "noName");
    screen->printLog("%s.channel:%d\n", prefix, m_channel);
    screen->printLog("%s.transpose:%d\n", prefix, m_transpose);
    screen->printLog("%s.rangeLower:%s\n", prefix, Midi::noteName(m_rangeLower));
    screen->printLog("%s.rangeUpper:%s\n", prefix, Midi::noteName(m_rangeUpper));
    screen->printLog("%s.nofHwParts:%d\n", prefix, m_hwPart.size());
    char nestedPrefix[100];
    for (size_t i=0; i<m_hwPart.size(); i++)
    {
        sprintf(nestedPrefix, "%s.hwPart%02d", prefix, 1+(int)m_hwPart[i]->m_number);
        m_hwPart[i]->dumpToLog(screen, nestedPrefix);
    }
}

void SetList::add(std::vector<Track*> &tracks, const char *trackName)
{
    int addIdx = -1;
    for (int i=0; i<(int)tracks.size(); i++)
    {
        if (strncasecmp(trackName, tracks[i]->m_name, strlen(trackName)) == 0)
        {
            addIdx = i;
            break;
        }
    }
    if (addIdx == -1)
    {
        printf("cannot find track %s by name\n", trackName);
        exit(1);
    }
    add(addIdx);
    //printf("added track %d to setlist: %s\n", addIdx, trackName);
}

void initTracks(std::vector<Track*> &tracks, SetList &setList)
{
#if 0
    const bool chain = true;
    tracks.push_back(new Track("Overture 1928"));
    tracks.push_back(new Track("Metropolis part I"));
    tracks.push_back(new Track("Erotomania"));
    tracks.push_back(new Track("Home"));
    tracks.push_back(new Track("Pull me under"));
    tracks.push_back(new Track("Hell's kitchen"));
    tracks.push_back(new Track("As I am"));
    tracks.push_back(new Track("Another Day"));
    tracks.push_back(new Track("Peruvian Skies", !chain, 3));
    tracks.push_back(new Track("Learning to live"));
    tracks.push_back(new Track("Fireblossom"));
    tracks.push_back(new Track("Under a glass moon"));
    tracks.push_back(new Track("Overture 1928 2", !chain, 1));
    Track *t = new Track("Lines in the sand 1/2");
    t->m_section[0]->m_noteOffLeave = false;
    t->m_section[1]->m_noteOffEnter = false;
    tracks.push_back(t);
    tracks.push_back(new Track("Lines in the sand 2/2", !chain, 2));
    tracks.push_back(new Track("A change of seasons 1/2"));
    tracks.push_back(new Track("A change of seasons 2/2"));
    tracks.push_back(new Track("Biaxxident"));
    tracks.push_back(new Track("Trial of tears"));
    tracks.push_back(new Track("Surrounded"));
    tracks.push_back(new Track("Take the time"));
    tracks.push_back(new Track("Only a matter of time"));
    t = new Track("The count of Tuscany");
    t->m_section.clear();
    Section *s = new Section("Intro, strings + EGuitar");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B4", 0, 24, "lower"));
    s->addPart(new SwPart("C5", "B9", 1, -12, "upper"));
    t->addSection(s);
    s = new Section("Intro 2, sound FX + strings");
    s->m_part.clear();
    s->addPart(new SwPart("F2", "B9", 5, 0, "upper"));
    s->addPart(new SwPart("C0", "E2", 6, 0, "lower"));
    t->addSection(s);
    s = new Section("Xylo - short");
    s->m_part.clear();
    s->addPart(new SwPart("C4", "B9", 2, -12, "upper"));
    s->addPart(new SwPart("C0", "B3", 5, 12, "lower"));
    t->addSection(s);

    s = new Section("unison");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 14, -12, "upper"));
    t->addSection(s);

    s = new Section("Harmoniser porta");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 9, 0, "upper"));
    SwPart *p = new SwPart("C0", "B9", 10, 1000 - 12, "lower");
    p->m_customTranspose[0] = 4; // c
    p->m_customTranspose[1] = 0;
    p->m_customTranspose[2] = 3; // d
    p->m_customTranspose[3] = 0;
    p->m_customTranspose[4] = 3; // e
    p->m_customTranspose[5] = 4; // f
    p->m_customTranspose[6] = 0;
    p->m_customTranspose[7] = 4; // g
    p->m_customTranspose[8] = 0;
    p->m_customTranspose[9] = 3; // a
    p->m_customTranspose[10] = 0;
    p->m_customTranspose[11] = 3; // b
    p->m_customTransposeOffset = 1;
    s->addPart(p);
    t->addSection(s);
    s = new Section("Fizz - unison");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 13, -12, "upper"));
    t->addSection(s);
    s = new Section("Xylo - 6x");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 2, +24, "upper"));
    t->addSection(s);
    s = new Section("Fizz - 4x");
    s->m_part.clear();
    s->addPart(new SwPart("C5", "B9", 3, -12, "upper"));
    s->addPart(new SwPart("C0", "B4", 7, +12, "lower"));
    t->addSection(s);
    s = new Section("Tron 1a");
    s->m_part.clear();
    s->addPart(new SwPart("C7", "B9", 4, -12, "upper"));
    s->addPart(new SwPart("C0", "B6", 5, 0, "lower"));
    t->addSection(s);

    s = new Section("Organ 1");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 15, +12, "upper"));
    t->addSection(s);

    s = new Section("Tron 1b");
    s->m_part.clear();
    s->addPart(new SwPart("C7", "B9", 4, -12, "upper"));
    s->addPart(new SwPart("C0", "B6", 5, 0, "lower"));
    t->addSection(s);

    s = new Section("bell-EG");
    s->m_part.clear();
    s->addPart(new SwPart("G5", "B9", 11, 0, "upper"));
    s->addPart(new SwPart("G5", "B9", 1, 0, "upper"));
    s->addPart(new SwPart("C0", "A5", 5, 0, "lower"));
    s->addPart(new SwPart("C0", "A5", 5, 12, "lower"));
    t->addSection(s);

    s = new Section("Xylo - 4x");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 2, +24, "upper"));
    t->addSection(s);
    s = new Section("Fizz - 3x");
    s->m_part.clear();
    s->addPart(new SwPart("C5", "B9", 3, -12, "upper"));
    s->addPart(new SwPart("C0", "B4", 7, +12, "lower"));
    t->addSection(s);
    s = new Section("Tron 2a");
    s->m_part.clear();
    s->addPart(new SwPart("C7", "B9", 4, -12, "upper"));
    s->addPart(new SwPart("C0", "B6", 5, 0, "lower"));
    t->addSection(s);
    s = new Section("Organ 2");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", 15, +12, "upper"));
    t->addSection(s);
    s = new Section("Tron 2b");
    s->m_part.clear();
    s->addPart(new SwPart("C7", "B9", 4, -12, "upper"));
    s->addPart(new SwPart("C0", "B6", 5, 0, "lower"));
    t->addSection(s);

    s = new Section("bell-EG (2)");
    s->m_part.clear();
    s->addPart(new SwPart("G5", "B9", 11, 0, "upper"));
    s->addPart(new SwPart("G5", "B9", 1, 0, "upper"));
    s->addPart(new SwPart("C0", "A5", 5, 0, "lower"));
    s->addPart(new SwPart("C0", "A5", 5, 12, "lower"));
    t->addSection(s);

    s = new Section("Fuzzy");
    s->m_part.clear();
    s->addPart(new SwPart("C5", "B9", 8, -12, "upper"));
    s->addPart(new SwPart("C0", "B4", 8, 24, "lower"));
    s->addPart(new SwPart("C5", "B9", 2, -12, "upper"));
    t->addSection(s);
    s = new Section("Fizz");
    s->m_part.clear();
    s->addPart(new SwPart("C5", "B9", 3, -12, "upper"));
    s->addPart(new SwPart("C0", "B4", 7, +12, "lower"));
    t->addSection(s);
    s = new Section("Xylo long unison");
    s->m_part.clear();
    s->addPart(new SwPart("C6", "B9", 2, -24, "upper"));
    s->addPart(new SwPart("C6", "B9", 5, -24, "all"));
    s->addPart(new SwPart("C0", "B5", 12, 0, "upper"));

#if 0
    p = new SwPart("C0", "B5", 5/*10*/, 1000 + 12, "lower");
    p->m_customTranspose[0] = -9; // c
    p->m_customTranspose[1] = -12;
    p->m_customTranspose[2] = -8; // d
    p->m_customTranspose[3] = -12;
    p->m_customTranspose[4] = -12; // e
    p->m_customTranspose[5] = -5; // f
    p->m_customTranspose[6] = -12;
    p->m_customTranspose[7] = -7; // g
    p->m_customTranspose[8] = -12;
    p->m_customTranspose[9] = -7; // a
    p->m_customTranspose[10] = -12;
    p->m_customTranspose[11] = -5; // b
    p->m_customTransposeOffset = 0;
    s->addPart(p);
#endif
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;

    t->addSection(s);
    s = new Section("String pad");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 5, 0, "upper");
    p->m_controllerRemap = new ControllerRemapVolReverse;
    s->addPart(p);
    p = new SwPart("C0", "B9", 13, 0, "upper");
    p->m_controllerRemap = new ControllerRemapVolQuadratic;
    s->addPart(p);
    t->addSection(s);

    t->m_chain = true;
    tracks.push_back(t);

    t = new Track("The count of Tuscany 2");
    t->m_section.clear();
    s = new Section("String pad bubbles");
    s->m_part.clear();
    p = new SwPart("C0", "B5", 0, 0, "upper");
    p->m_controllerRemap = new ControllerRemapVolReverse;
    s->m_previousSection = 20;
    s->m_previousTrack = 22;
    s->addPart(p);
    p = new SwPart("C0", "B5", 1, 0, "upper");
    p->m_controllerRemap = new ControllerRemapVolQuadratic;
    s->addPart(p);
    p = new SwPart("C6", "B9", 2, 0, "upper");
    s->addPart(p);

    t->addSection(s);

    s = new Section("Piano + strings");
    s->m_part.clear();
    p = new SwPart("C7", "B9", 3, -24, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B6", 4, 0, "upper");
    s->addPart(p);

    t->addSection(s);

    s = new Section("Strings");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 5, 0, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 5, -12, "upper");
    s->addPart(p);
    p = new SwPart("C0", "B9", 7, 0, "choir");
    s->addPart(p);

    t->addSection(s);

    s = new Section("Final");
    s->m_part.clear();
    p = new SwPart("C7", "B9", 3, -24, "piano");
    s->addPart(p);
    p = new SwPart("F2", "B6", 4, 0, "upper");
    s->addPart(p);
    p = new SwPart("C0", "E2", 6, +32, "swamp");
    s->addPart(p);

    t->addSection(s);

    t->m_chain = true;
    tracks.push_back(t);

    t = new Track("The Mirror");
    t->m_section.clear();
    s = new Section("String pad intro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Organ");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 1, 12, "organ");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Organ + Guitar unison");
    s->m_part.clear();
    p = new SwPart("C0", "B4", 1, 12, "organ");
    s->addPart(p);
    p = new SwPart("C5", "B9", 2, -24, "solo");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Thin strings");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Piano solo 1");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "piano");
    s->addPart(p);
    p = new SwPart("C5", "B9", 4, +12, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 4, +36, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 4, +48, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Thin strings 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("String pad intro 2a");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Piano solo delay");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "piano");
    s->addPart(p);
    p = new SwPart("C5", "B9", 4, +12, "piano");
    s->addPart(p);

    p = new SwPart("F2", "B4", 4, +36, "piano");
    s->addPart(p);
    p = new SwPart("F2", "B4", 4, +48, "piano");
    s->addPart(p);
    p = new SwPart("C0", "E2", 9, 0, "sample");
    s->addPart(p);
    p->m_toggler.enable();
    t->addSection(s);

    s = new Section("EPiano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 5, 0, "EPiano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Piano solo reprise");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "piano");
    s->addPart(p);
    p = new SwPart("C5", "B9", 4, +12, "piano");
    s->addPart(p);

    p = new SwPart("C0", "B4", 4, +36, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 4, +48, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("String pad intro 2b");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Piano bridge");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 8, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B9", 8, -12, "piano");
    s->addPart(p);

    p = new SwPart("C0", "B9", 7, -12, "choir");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Solo delay");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "piano");
    s->addPart(p);
    p = new SwPart("C5", "B9", 4, +12, "piano");
    s->addPart(p);

    p = new SwPart("C0", "B4", 4, +36, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 4, +48, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("outtro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 6, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B9", 6, +12, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B9", 0, 0, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 0, -12, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Solo delay (live ext)");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "piano");
    s->addPart(p);
    p = new SwPart("C5", "B9", 4, +12, "piano");
    s->addPart(p);

    p = new SwPart("C0", "B4", 4, +36, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 4, +48, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Organ finale");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 1, 12, "organ");
    s->addPart(p);
    t->addSection(s);


    t->m_chain = true;
    tracks.push_back(t);

    t = new Track("Scarred");
    t->m_section.clear();
    s = new Section("String pad intro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 12, "strings");
    s->addPart(p);
    //p = new SwPart("C0", "B2", 1, 0, "piano");
    //s->addPart(p);
    t->addSection(s);

    s = new Section("String pad + arp");
    s->m_part.clear();
    p = new SwPart("C0", "E5", 0, 12, "strings");
    s->addPart(p);
    p = new SwPart("C0", "E5", 0, 24, "strings");
    s->addPart(p);
    p = new SwPart("F5", "B9", 3, -12, "arp");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Thin string");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 9, -12, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 9, 12, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 9, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Organ");
    s->m_part.clear();
    p = new SwPart("C0", "B5", 2, 0, "organ");
    s->addPart(p);
    p = new SwPart("C6", "B6", 5, -24, "tubular");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, -12, "strings");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Thin string");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 9, -12, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 9, 12, "strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 9, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Organ");
    s->m_part.clear();
    p = new SwPart("C0", "B5", 2, 0, "organ");
    s->addPart(p);
    p = new SwPart("C6", "B6", 5, -24, "tubular");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, -12, "strings");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, 0, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("plain string pad intro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 12, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("filtered string pad");
    s->m_part.clear();
    p = new SwPart("C0", "A4", 7, 12, "filtered strings");
    s->addPart(p);
    p = new SwPart("B4", "B9", 8, -12, "wah strings");
    s->addPart(p);
    p = new SwPart("B5", "B9", 7, +12, "filtered strings");
    s->addPart(p);
    p = new SwPart("C7", "B9", 0, -12, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("solo bidge");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 12, "strings");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Unison solo 1");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, -12, "thin strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 6, 0, "thin strings");
    s->addPart(p);
    t->addSection(s);
    s = new Section("Unison solo 2");
    s->m_part.clear();
    p = new SwPart("C4", "B9", 4, 1000-12, "thin strings");
    p->m_customTranspose[0] = 0; // c
    p->m_customTranspose[1] = -3;
    p->m_customTranspose[2] = -3; // d
    p->m_customTranspose[3] = 0;
    p->m_customTranspose[4] = -3; // e
    p->m_customTranspose[5] = 0; // f
    p->m_customTranspose[6] = -4;
    p->m_customTranspose[7] = 0; // g
    p->m_customTranspose[8] = -4;
    p->m_customTranspose[9] = 0; // a
    p->m_customTranspose[10] = -4;
    p->m_customTranspose[11] = -3; // b
    p->m_customTransposeOffset = -24;
    s->addPart(p);
    p = new SwPart("C4", "B9", 6, -12, "thin strings");
    s->addPart(p);
    t->addSection(s);
    s = new Section("Unison solo cadence");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, -24, "thin strings");
    s->addPart(p);
    p = new SwPart("C0", "B9", 6, -12, "thin strings");
    s->addPart(p);

    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;

    t->addSection(s);

    t->m_chain = true;
    tracks.push_back(t);


    t = new Track("Scarred Solo");
    t->m_section.clear();
    s = new Section("String pad intro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "Burning Lead");
    s->addPart(p);
    //p = new SwPart("C0", "B2", 1, 0, "piano");
    //s->addPart(p);
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()-1;
    t->addSection(s);
    t->m_chain = true;
    tracks.push_back(t);

    t = new Track("About to crash");
    t->m_section.clear();
    s = new Section("piano intro");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    //p = new SwPart("C0", "B2", 1, 0, "piano");
    //s->addPart(p);
    t->addSection(s);

    s = new Section("piano + lead");
    s->m_part.clear();
    p = new SwPart("G#5", "B9", 0, -12, "piano");
    s->addPart(p);
    p = new SwPart("C0", "G5", 1, +24, "lead");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano + strings");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "C6", 3, 0, "strings", true);
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano + voice");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "C7", 2, +12, "voice", true);
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano + strings 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "C7", 3, 0, "strings", false);
    s->addPart(p);
    t->addSection(s);

    s = new Section("syn brass solo");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, 0, "syn brass");
    s->addPart(p);
    p = new SwPart("F3", "B9", 4, 4, "syn brass");
    s->addPart(p);
    p = new SwPart("D#7", "B9", 5, 0, "syn brass");
    s->addPart(p);
    t->addSection(s);

    s = new Section("organ");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 10, -12, "organ");
    s->addPart(p);
    t->addSection(s);

    s = new Section("syn string");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 6, 0, "syn string");
    s->addPart(p);
    p = new SwPart("C0", "B9", 6, 12, "syn string");
    s->addPart(p);
    p = new SwPart("C0", "B9", 7, -12, "syn string");
    s->addPart(p);
    t->addSection(s);
    //tracks.push_back(t);

    s = new Section("syn brass solo 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, 0, "syn brass");
    s->addPart(p);
    p = new SwPart("F3", "B9", 4, 4, "syn brass");
    s->addPart(p);
    p = new SwPart("D#7", "B9", 5, 0, "syn brass");
    s->addPart(p);
    t->addSection(s);

    s = new Section("organ 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 10, -12, "organ");
    s->addPart(p);
    t->addSection(s);

    s = new Section("syn string");
    s->m_part.clear();
    p = new SwPart("C0", "F6", 6, 12, "syn string");
    s->addPart(p);
    p = new SwPart("C0", "F6", 6, 24, "syn string");
    s->addPart(p);
    p = new SwPart("C0", "F6", 7, 0, "syn string");
    s->addPart(p);
    p = new SwPart("G6", "B9", 8, -12, "vox");
    s->addPart(p);
    p = new SwPart("G6", "B9", 8, -24, "vox");
    s->addPart(p);
    t->addSection(s);
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;

    t->m_chain = true;

    tracks.push_back(t);

    t = new Track("The test that stumped them all");
    t->m_section.clear();
#if 0
    s = new Section("fast lead1");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 8, 0, "Burning Lead");
    s->addPart(p);
    t->addSection(s);
#endif

    s = new Section("xylo lead");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "xylo Lead", false, new Transposer(12));
    s->addPart(p);
    t->addSection(s);

    s = new Section("buzz solo");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 5, 0, "buzz solo");
    s->addPart(p);
    t->addSection(s);

    s = new Section("organ");
    s->m_part.clear();
    p = new SwPart("C0", "B4", 1, 24, "organ Lead");
    s->addPart(p);
    p = new SwPart("C5", "B7", 2, -24, "12 string");
    s->addPart(p);
    p = new SwPart("C8", "B9", 3, 0, "Crowd Cheering");
    s->addPart(p);
    t->addSection(s);

    s = new Section("guitar solo");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 5, 0, "buzz solo");
    s->addPart(p);
    p = new SwPart("C0", "B9", 7, +12, "buzz solo");
    s->addPart(p);
    t->addSection(s);

    s = new Section("solo");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, 0, "solo");
    s->addPart(p);
    t->addSection(s);

    s = new Section("solo 2");
    s->m_part.clear();
    p = new SwPart("E3", "B9", 5, 0, "buzz solo");
    s->addPart(p);
    p = new SwPart("C0", "D3", 6, 0, "bell attack");
    s->addPart(p);
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;

    t->addSection(s);

    t->m_chain = true;
    tracks.push_back(t);

    t = new Track("Goodnight Kiss");
    t->m_section.clear();
    s = new Section("phase pad");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "phase pad");
    s->addPart(p);
    p = new SwPart("A4", "B9", 5, 0, "EP stack");
    s->addPart(p);
    p = new SwPart("C0", "G4", 1, 0, "string pad");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("string piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B9", 3, 0, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("orch piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B9", 6, 0, "FS windwood");
    s->addPart(p);
    p = new SwPart("C0", "B9", 3, 0, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("string piano high");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    p = new SwPart("G4", "B9", 6, 0, "FS windwood");
    s->addPart(p);
    p = new SwPart("C0", "B9", 7, 12, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("choir");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 4, 0, "choir");
    s->addPart(p);
    p = new SwPart("C0", "B4", 3, 0, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("walking string");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 7, 0, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("arpeggio");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 8, 0, "ethno keys");
    s->addPart(p);
    p = new SwPart("C0", "B9", 3, 0, "string pad 2");
    s->addPart(p);
    t->addSection(s);

    t->m_chain = true;
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;
    tracks.push_back(t);

    t = new Track("Solitary shell");
    t->m_section.clear();
    s = new Section("soft square lead");
    s->m_part.clear();
    p = new SwPart("B4", "B9", 0, 0, "square lead");
    s->addPart(p);
    p = new SwPart("G4", "A4", 1, +36, "square lead");
    s->addPart(p);
    p = new SwPart("C0", "F4", 2, +36, "arpeggio");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("bells");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B6", 9, 12, "bells");
    s->addPart(p);
    t->addSection(s);

    s = new Section("soft square lead 2");
    s->m_part.clear();
    p = new SwPart("B4", "B9", 0, 0, "square lead");
    s->addPart(p);
    p = new SwPart("G4", "A4", 1, +36, "square lead");
    s->addPart(p);
    p = new SwPart("C0", "F4", 2, +36, "arpeggio");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("bells 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B6", 9, 12, "bells");
    s->addPart(p);
    t->addSection(s);

    s = new Section("brassy");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano 3");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 3, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("brassy 2");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 4, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    t->m_chain = true;
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;
    tracks.push_back(t);
/*
 * ************************************************************
 */
    t = new Track("Reprise");
    t->m_section.clear();
    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("soft brass + choir");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 0, -12, "soft brass");
    s->addPart(p);
    p = new SwPart("C0", "B4", 1, +24, "choir");
    s->addPart(p);
    p = new SwPart("C5", "B9", 2, -12, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 2, 24, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("pizzicato");
    s->m_part.clear();
    p = new SwPart("C4", "B9", 3, -12, "pizzicato");
    s->addPart(p);
    p = new SwPart("C4", "B9", 4, -12, "pad");
    s->addPart(p);
    p = new SwPart("C0", "B3", 3, +24, "pizzicato");
    s->addPart(p);
    p = new SwPart("C0", "B3", 4, +24, "pad");
    s->addPart(p);
    t->addSection(s);

    s = new Section("pizzicato + solo");
    s->m_part.clear();
    p = new SwPart("C4", "B9", 3, -12, "pizzicato");
    s->addPart(p);
    p = new SwPart("C4", "B9", 4, -12, "pad");
    s->addPart(p);
    p = new SwPart("C4", "B9", 6, 0, "soft lead");
    s->addPart(p);
//    p = new SwPart("C0", "B3", 3, +24, "pizzicato");
//    s->addPart(p);
    p = new SwPart("C4", "F#5", 14, 0, "EP");
    s->addPart(p);
    p = new SwPart("C0", "B3", 4, +24, "pad");
    s->addPart(p);
    p = new SwPart("C0", "B3", 7, +24, "soft lead");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano+string+lead");
    s->m_part.clear();
    p = new SwPart("C0", "G4", 2, +24, "piano2");
    s->addPart(p);
    p = new SwPart("C0", "G4", 5, +24, "pad");
    s->addPart(p);
    p = new SwPart("A4", "B9", 8, 0, "lead");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano+string+choir");
    s->m_part.clear();
    p = new SwPart("C0", "G4", 2, +24, "piano2");
    s->addPart(p);
    p = new SwPart("C0", "G4", 5, +24, "pad");
    s->addPart(p);
    p = new SwPart("A4", "B9", 1, -12, "choir");
    s->addPart(p);
    p = new SwPart("A4", "B9", 9, 0, "bright string");
    s->addPart(p);
    p = new SwPart("A4", "B9", 1, 0, "choir");
    s->addPart(p);
    p = new SwPart("A4", "B9", 9, +12, "bright string");
    s->addPart(p);
    p = new SwPart("A4", "B9", 1, +12, "choir");
    s->addPart(p);
    p = new SwPart("A4", "B9", 2, +12, "piano2");
    s->addPart(p);
    t->addSection(s);

    s = new Section("tubular + picolo");
    s->m_part.clear();
    p = new SwPart("C0", "B3", 10, +24, "tubular");
    s->addPart(p);
    p = new SwPart("C4", "G5", 11, +36, "picolo");
    s->addPart(p);
    p = new SwPart("A5", "C9", 12, +12, "EP");
    s->addPart(p);
    p = new SwPart("A5", "C9", 12, -12, "EP");
    s->addPart(p);
    p = new SwPart("A5", "C9", 9, -12, "bright string");
    s->addPart(p);
    p = new SwPart("A5", "C9", 9, 0, "bright string");
    s->addPart(p);
    t->addSection(s);

    s = new Section("lead 2");
    s->m_part.clear();
    p = new SwPart("D4", "B9", 8, 0, "lead");
    s->addPart(p);
    p = new SwPart("C0", "C4", 1, 24, "choir");
    s->addPart(p);
    p = new SwPart("C0", "C4", 9, 12, "bright string");
    s->addPart(p);
    t->addSection(s);

    s = new Section("thin string + piano");
    s->m_part.clear();
    p = new SwPart("D4", "B9", 13, 0, "thin string");
    s->addPart(p);
    p = new SwPart("C0", "C4", 2, 36, "piano");
    s->addPart(p);
    p = new SwPart("C0", "C4", 9, 24, "bright string");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano solo");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 2, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "G#5", 9, 0, "bright string");
    s->addPart(p);
    t->addSection(s);

    s = new Section("soft brass");
    s->m_part.clear();
    p = new SwPart("F#5", "B9", 0, -12, "soft brass");
    s->addPart(p);
    p = new SwPart("F#5", "B9", 0, -12+4, "soft brass");
    s->addPart(p);
    p = new SwPart("C0", "F5", 5, +12, "pad");
    s->addPart(p);
    t->addSection(s);

    s = new Section("soft brass 2");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 0, -24, "soft brass");
    s->addPart(p);
    p = new SwPart("C5", "B9", 9, -24, "bright string");
    s->addPart(p);
    p = new SwPart("C5", "B9", 9, -12, "bright string");
    s->addPart(p);
    p = new SwPart("C5", "B9", 1, -12, "choir");
    s->addPart(p);
    p = new SwPart("C0", "B4", 1, 12, "choir");
    s->addPart(p);
    p = new SwPart("C0", "B4", 9, +12, "bright string");
    s->addPart(p);
    t->addSection(s);

    t->m_chain = true;
    s->m_nextSection = 0;
    s->m_nextTrack = tracks.size()+1;
    tracks.push_back(t);
/*
 * ************************************************************
 */
    t = new Track("Losing Time");
    t->m_section.clear();
    s = new Section("grand strings");
    s->m_part.clear();
    p = new SwPart("C6", "B9", 0, -12, "high");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, -24, "high");
    s->addPart(p);
    p = new SwPart("C0", "B5", 1, +12, "high");
    s->addPart(p);
    t->addSection(s);

    tracks.push_back(t);
/*
 * ************************************************************
 */
    t = new Track("Fatal Tragedy");
    t->m_section.clear();

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("choir");
    s->m_part.clear();
    p = new SwPart("C0", "B5", 1, 0, "choir0");
    s->addPart(p);
    p = new SwPart("C0", "B5", 1, 12, "choir1");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("organ + EP");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 2, 0, "organ");
    s->addPart(p);
    p = new SwPart("C0", "B4", 3, 0, "EP");
    s->addPart(p);
    p = new SwPart("C0", "B4", 0, -12, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("organ unison");
    s->m_part.clear();
    p = new SwPart("C5", "B9", 0, 0, "piano");
    s->addPart(p);
    p = new SwPart("C0", "B4", 5, +36, "organ");
    s->addPart(p);
    t->addSection(s);

    s = new Section("choir");
    s->m_part.clear();
    p = new SwPart("C0", "B5", 1, 0, "choir0");
    s->addPart(p);
    p = new SwPart("C0", "B5", 1, 12, "choir1");
    s->addPart(p);
    p = new SwPart("C6", "B9", 0, 0, "piano");
    s->addPart(p);
    t->addSection(s);

    s = new Section("piano");
    s->m_part.clear();
    p = new SwPart("C0", "B9", 0, 0, "piano");
    s->addPart(p);
    t->addSection(s);

        int cc = 4;

    s = new Section("clavi solo");
    s->m_part.clear();
    p = new SwPart("C0", "B5", cc, 12, "clavi lo");
    s->addPart(p);
    p = new SwPart("C6", "B9", cc, -12, "clavi hi");
    s->addPart(p);
    t->addSection(s);

    s = new Section("Harmoniser clavi");
    s->m_part.clear();
    s->addPart(new SwPart("C0", "B9", cc, 0, "upper"));
    p = new SwPart("C0", "B9",  cc, 1000 - 12, "lower");
    p->m_customTranspose[0] = 4; // c
    p->m_customTranspose[1] = 5;
    p->m_customTranspose[2] = 0; // d
    p->m_customTranspose[3] = 0;
    p->m_customTranspose[4] = 3; // e
    p->m_customTranspose[5] = 0; // f
    p->m_customTranspose[6] = 4;
    p->m_customTranspose[7] = 4; // g
    p->m_customTranspose[8] = 0;
    p->m_customTranspose[9] = 3; // a
    p->m_customTranspose[10] = 3;
    p->m_customTranspose[11] = 5; // b
    p->m_customTransposeOffset = 0;
    s->addPart(p);
    s->addPart(new SwPart("C0", "B9", cc, -12, "upper"));
    p = new SwPart("C0", "B9",  cc, 1000 - 24, "lower");
    p->m_customTranspose[0] = 4; // c
    p->m_customTranspose[1] = 5;
    p->m_customTranspose[2] = 0; // d
    p->m_customTranspose[3] = 0;
    p->m_customTranspose[4] = 3; // e
    p->m_customTranspose[5] = 0; // f
    p->m_customTranspose[6] = 4;
    p->m_customTranspose[7] = 4; // g
    p->m_customTranspose[8] = 0;
    p->m_customTranspose[9] = 3; // a
    p->m_customTranspose[10] = 3;
    p->m_customTranspose[11] = 5; // b
    p->m_customTransposeOffset = 0;
    s->addPart(p);
    t->addSection(s);

    s = new Section("clavi solo 2");
    s->m_part.clear();
    p = new SwPart("C0", "B5", cc, 12, "clavi lo");
    s->addPart(p);
    p = new SwPart("C6", "B9", cc, -12, "clavi hi");
    s->addPart(p);
    t->addSection(s);

    t->m_chain = true;
    tracks.push_back(t);
#endif
/*
 * ************************************************************
 */
    // set 1
    setList.add(tracks, "Overture 1928 2");
    setList.add(tracks, "Overture 1928");
    setList.add(tracks, "Lines in the sand 1/2");
    setList.add(tracks, "Lines in the sand 2/2");
    setList.add(tracks, "Another day");
    setList.add(tracks, "Scarred");
    setList.add(tracks, "The Mirror");

    // set 2
    setList.add(tracks, "About to crash");
    setList.add(tracks, "Pull me under");
    setList.add(tracks, "Erotomania");
    setList.add(tracks, "Overture 1928"); // voices

    // set 3
    setList.add(tracks, "Home");
    setList.add(tracks, "As i am");
    setList.add(tracks, "Metropolis part I");
    setList.add(tracks, "Peruvian skies");
    setList.add(tracks, "Learning to live");

    // encore
    setList.add(tracks, "Under a glass moon");
    setList.add(tracks, "The count of Tuscany");
#if 0
    // set 1
    setList.add(tracks, "Overture 1928 2");
    setList.add(tracks, "Overture 1928");
    setList.add(tracks, "Erotomania");
    setList.add(tracks, "Overture 1928"); // voices
    setList.add(tracks, "Under a glass moon");
    setList.add(tracks, "Another day");
    setList.add(tracks, "Metropolis part I");
    setList.add(tracks, "Trial of tears");
    setList.add(tracks, "Only a matter of time");
    // set 2
    setList.add(tracks, "Home");
    setList.add(tracks, "Pull me under");
    setList.add(tracks, "Take the time");
    setList.add(tracks, "As i am");
    setList.add(tracks, "Lines in the sand 1/2");
    setList.add(tracks, "Lines in the sand 2/2");
    setList.add(tracks, "Peruvian skies");
    //setList.add(tracks, "Overture 1928 2"); // wait for sleep
    //setList.add(tracks, "A change of seasons 1/2");
    //setList.add(tracks, "A change of seasons 2/2");
    //setList.add(tracks, "Hell's kitchen");
    setList.add(tracks, "Learning to live");
    // encore
    setList.add(tracks, "The count of Tuscany");
    //for (int i=0; i<tracks.size(); i++) tracks[i]->dumpToLog();
#endif
    //exit(1);
}
