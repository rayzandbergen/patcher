/*! \file sequencer.cpp
 *  \brief Contains an unused sequencer object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "sequencer.h"
#include <ctime>
//! \brief Unused.
void Sequencer::enable()
{
    if (m_enabled)
        disable();
    m_fp = new std::ofstream(fileName().c_str());
    m_enabled = true;
}

//! \brief Unused.
void Sequencer::disable()
{
    if (m_enabled)
    {
        delete m_fp;
        m_enabled = false;
    }
}

//! \brief Unused.
bool Sequencer::toggle()
{
    if (m_enabled)
        disable();
    else
        enable();
    return m_enabled;
}

//! \brief Unused.
std::string Sequencer::fileName() const
{
    std::time_t result = std::time(0);
    char buf[80];
    std::strftime(buf, sizeof(buf), "seq-%Y%m%d-%H%M%S.txt", std::localtime(&result));
    return std::string(buf);
}

