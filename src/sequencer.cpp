/*! \file sequencer.cpp
 *  \brief Contains an unused sequencer object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "sequencer.h"
#include <ctime>
void Sequencer::enable()
{
    if (m_enabled)
        disable();
    m_fp = new std::ofstream(fileName().c_str());
    m_enabled = true;
}

void Sequencer::disable()
{
    if (m_enabled)
    {
        delete m_fp;
        m_enabled = false;
    }
}

bool Sequencer::toggle()
{
    if (m_enabled)
        disable();
    else
        enable();
    return m_enabled;
}

std::string Sequencer::fileName() const
{
    std::time_t result = std::time(0);
    char buf[80];
    std::strftime(buf, sizeof(buf), "seq-%Y%m%d-%H%M%S.txt", std::localtime(&result));
    return std::string(buf);
}

