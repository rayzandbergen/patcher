/*! \file sequencer.h
 *  \brief Contains an unused sequencer object.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef SEQUENCER_H
#define SEQUENCER_H
#include <iostream>
#include <fstream>
//! \brief This class is not used. \todo Implement a sequencer.
class Sequencer
{
private:
    std::ofstream *m_fp;            //!< File stream to write to.
    std::string fileName() const;   //!< Return the current file name.
    bool m_enabled;                 //!< Enable switch.
public:
    void enable();                  //!< Enable the \a Sequencer.
    void disable();                 //!< Disable the \a Sequencer.
    bool toggle();                  //!< Toggle the on/off switch of the \a Sequencer.
    Sequencer(): m_fp(0), m_enabled(false) { } //!< Query enable switch.
};
#endif //SEQUENCER_H
