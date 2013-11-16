#include "undupparts.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

class NewPart
{
public:
    std::string m_name;
    int m_channel;
};

class NewSection
{
public:
    std::string m_name;
    std::vector<NewPart> m_part;
    bool m_noteOffEnter, m_noteOffLeave;
};

void chompTrailingSpace(std::string &s)
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
}

void undupParts(const Fantom::Performance *perfList, const TrackList &trackList)
{
    std::vector<NewSection> sectionList;
    std::ofstream of("undup.txt");
    for (size_t i = 0; i < 22; i++)
    {
        const Fantom::Performance *perf = perfList + i;
        const Track *track = trackList[i];
        of << "    <track name=\"" << track->m_name << "\"";
        int startSection = track->m_startSection;
        if (startSection != 0)
            of << " startSection=\"" << startSection << "\"";
        of << ">\n";
        sectionList.resize(track->nofSections());
        for (size_t s = 0; s < sectionList.size(); s++)
        {
            sectionList[s].m_part.clear();
            sectionList[s].m_noteOffEnter = track->m_section[s]->m_noteOffEnter;
            sectionList[s].m_noteOffLeave = track->m_section[s]->m_noteOffLeave;
        }
        of << "<![CDATA[\n";
        for (size_t hp = 0; hp<16; hp++)
        {
            const Fantom::Part *part = &perf->m_part[hp];
            size_t sIdx = (size_t)part->m_channel;
            if (part->m_channel != hp)
            {
                of << "    part " << (hp+1) << " is on channel " << (1+(int)part->m_channel) << ", reset to " << (hp+1) << "\n";
            }
            if (sIdx < sectionList.size())
            {
                NewPart newPart;
                newPart.m_channel = hp;
                newPart.m_name = part->m_patch.m_name;
                chompTrailingSpace(newPart.m_name);
                sectionList[sIdx].m_part.push_back(newPart);
            }
        }
        of << "]]>\n";
        for (size_t s = 0; s < sectionList.size(); s++)
        {
            if (sectionList[s].m_part.empty())
                continue;
            of << "      <section";
            if (sectionList[s].m_part.size() != 1)
            {
                std::stringstream sstream;
                sstream << "Section " << (s+1);
                sectionList[s].m_name = sstream.str();
            }
            else
            {
                sectionList[s].m_name = sectionList[s].m_part[0].m_name;
            }
            of << " name=\"" << sectionList[s].m_name << "\">\n";
            bool e = sectionList[s].m_noteOffEnter;
            bool l = sectionList[s].m_noteOffLeave;
            if (!e || !l)
            {
                of << "      <noteOff";
                if (!e)
                    of << " enter=\"false\"";
                if (!l)
                    of << " leave=\"false\"";
                of << "      />\n";
            }
            for (size_t p = 0; p < sectionList[s].m_part.size(); p++)
            {
                of << "        <part channel=\"" <<
                   (1+sectionList[s].m_part[p].m_channel) << "\"" << 
                   " name=\"" << sectionList[s].m_part[p].m_name << "\">\n";
                of << "          <range lower=\"C0\" upper=\"G10\"/>\n"
                      "          <transpose offset=\"0\"/>\n"
                      "        </part>\n";
            }
            of << "      </section>\n";
        }
        of << "    </track>\n\n";
    }
}
