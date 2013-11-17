#include "undupparts.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <string.h>

void sanitise(std::string &s)
{
    while (s.find_last_of(" ")+1 == s.size())
    {
        s = s.substr(0, s.size()-1);
    }
    for(;;)
    {
        size_t n = s.find("&");
        if (n == s.npos)
            break;
        s.replace(n, 1, "&amp;");
            break;
    }
    for(;;)
    {
        size_t n = s.find("\"");
        if (n == s.npos)
            break;
        s.replace(n, 1, "&quot;");
    }
}

std::string sanitise(const char *s)
{
    std::string rv(s);
    sanitise(rv);
    return rv;
}

void undupParts(const Fantom::Performance *perfList, const TrackList &trackList)
{
    std::ofstream of("undup.txt");
    for (size_t i = 0; i < 22; i++)
    {
        const Fantom::Performance *perf = perfList + i;
        const Track *track = trackList[i];
        of << "    <track name=\"" << sanitise(track->m_name) << "\"";
        int startSection = track->m_startSection;
        if (startSection != 0)
            of << " startSection=\"" << startSection << "\"";
        of << ">\n";
        for (SectionList::const_iterator si = track->m_section.begin(); 
                si != track->m_section.end(); si++)
        {
            const Section *section = *si;
            of << "      <section";
            of << " name=\"" << sanitise(section->m_name) << "\">\n";
            bool ne = section->m_noteOffEnter;
            bool nl = section->m_noteOffLeave;
            if (!ne || !nl)
            {
                of << "      <noteOff";
                if (!ne)
                    of << " enter=\"false\"";
                if (!nl)
                    of << " leave=\"false\"";
                of << "      />\n";
            }
            for (size_t p = 0; p < section->m_part.size(); p++)
            {
                const SwPart *swPart = section->m_part[p];
                const Fantom::Part *hwPart = &perf->m_part[swPart->m_channel];
                int l = hwPart->m_keyRangeLower;
                int u = hwPart->m_keyRangeUpper;
                int fl = hwPart->m_fadeWidthLower;
                int fu = hwPart->m_fadeWidthUpper;
                char lString[20];
                char uString[20];
                Midi::noteName(l, lString);
                Midi::noteName(u, uString);
                if (l > 0 || u < 127)
                {
                    if (fl != 0 || fu != 0)
                    {
                        of << "<![CDATA[\n";
                        of << "  range is " << lString << " - " << uString <<
                        " but fadewidth is " << fl << "," << fu << "\n";
                        of << "]]>\n";
                        strcpy(lString, "C0");
                        strcpy(uString, "G10");
                    }
                }
                of << "        <part channel=\"" <<
                   (1+swPart->m_channel) << "\"" << 
                   " name=\"" << sanitise(swPart->m_name) << "\">\n";
                of << "          <range lower=\"" << lString << "\" upper=\"" << uString << "\"/>\n"
                      "          <transpose offset=\"0\"/>\n"
                      "        </part>\n";
            }
            of << "      </section>\n";
        }
        of << "    </track>\n\n";
    }
}
