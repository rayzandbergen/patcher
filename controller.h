#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H
#include "stdint.h"
#include "patcher.h"
#include "midi.h"

class ControllerRemap {
protected:
    uint8_t m_from;
    uint8_t m_to;
    Real m_x0, m_y0, m_x1, m_y1;
public:
    void value(uint8_t controller, uint8_t val, 
        uint8_t *controllerOut, uint8_t *valOut) const;
    virtual Real interpolate(Real x, Real x0, Real y0, 
        Real x1, Real y1) const;
    ControllerRemap(
        uint8_t from = 255, uint8_t to = 255,
        uint8_t x0 = 0,     uint8_t y0 = 0,
        uint8_t x1 = 127,   uint8_t y1 = 127):
        m_from(from), m_to(to), m_x0(x0), m_y0(y0),
        m_x1(x1), m_y1(y1) { }
};

class ControllerRemapVolQuadratic: public ControllerRemap {
public:
    ControllerRemapVolQuadratic(): ControllerRemap(
        MidiController::continuousController16, 
        MidiController::mainVolume 
    ) { }
    virtual Real interpolate(Real x, Real x0, Real y0, 
        Real x1, Real y1) const;
};

class ControllerRemapVolReverse: public ControllerRemap {
public:
    ControllerRemapVolReverse(): ControllerRemap(
        MidiController::continuousController16, 
        MidiController::mainVolume,
        0, 127, 127, 0) { };
};

#endif
