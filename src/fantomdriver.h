/*! \file fantomdriver.h
 *  \brief Contains classes to retrieve and set parameters in the Fantom.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef FANTOM_DRIVER_H
#define FANTOM_DRIVER_H
#include <stdint.h>
#include "screen.h"
#include "mididef.h"
#include "mididriver.h"
#include "fantomdef.h"

//! \brief Namespace for Fantom driver objects.
namespace Fantom
{

/*! \brief Upload and download parameters from and to the Fantom.
 */
class Driver
{
    Midi::Driver *m_midi;       //!< A MIDI driver object.
    void setParam(const uint32_t addr, const uint32_t length, const uint8_t *data);
    void getParam(const uint32_t addr, const uint32_t length, uint8_t *data);
public:
    /* \brief Constructor
     *
     * Constructs an empty Driver object.
     */
    //! \brief Contruct a default Driver object.
    Driver(Midi::Driver *midi): m_midi(midi) { }
    void setVolume(uint8_t part, uint8_t val);
    void getPartParams(Part *p, int idx);
    void getPatchName(char *s, int idx);
    void getPerformanceName(char *s);
    void setPerformanceName(const char *s);
    void setPartName(int part, const char *s);
    void selectPerformance(uint8_t performanceIndex) const;
    void selectPerformanceFromMemCard() const;
    void download(WINDOW *win, Fantom::PerformanceList &performanceList, size_t nofPerformances);
};


} // Fantom namespace
#endif // FANTOM_DRIVER_H
