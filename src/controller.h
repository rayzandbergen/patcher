/*! \file controller.h
 *  \brief Contains an object remap MIDI Controller messages.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H
#include "stdint.h"
#include "patcher.h"
#include "midi.h"

/*! \brief Remaps a single controller to a new controller, with its value interpolated to a new value.
 */
class ControllerRemap {
protected:
    uint8_t m_from;                 //!<    Controller to map from.
    uint8_t m_to;                   //!<    Controller to map to.
    Real m_x0;                      //!<    Interpolation 'from' value x0.
    Real m_y0;                      //!<    Interpolation 'from' value y0.
    Real m_x1;                      //!<    Interpolation 'from' value x1.
    Real m_y1;                      //!<    Interpolation 'from' value y1.
public:
    //! \brief The name of the controller, so the configuration can refer to it.
    virtual const char *name() const { return "default"; }
    void value(uint8_t controller, uint8_t val,
        uint8_t *controllerOut, uint8_t *valOut) const;
    virtual Real interpolate(Real x, Real x0, Real y0,
        Real x1, Real y1) const;
    //! \brief Constructs a ControllerRemap that does nothing.
    ControllerRemap(
        uint8_t from = 255, uint8_t to = 255,
        uint8_t x0 = 0,     uint8_t y0 = 0,
        uint8_t x1 = 127,   uint8_t y1 = 127):
        m_from(from), m_to(to), m_x0(x0), m_y0(y0),
        m_x1(x1), m_y1(y1) { }
};

/*! \brief Remaps continuous controller 16 to volume, an replaces its value curve with a parabola.
 */
class ControllerRemapVolQuadratic: public ControllerRemap {
public:
    virtual const char *name() const { return "volQuadratic"; }
    ControllerRemapVolQuadratic(): ControllerRemap(
        Midi::continuousController16,
        Midi::mainVolume
    ) { }
    virtual Real interpolate(Real x, Real x0, Real y0,
        Real x1, Real y1) const;
};

/*! \brief Remaps continuous controller 16 to volume, an replaces its value curve with an inverted parabola, used for crossfades with \a ControllerRemapVolQuadratic.
 */
class ControllerRemapVolReverse: public ControllerRemap {
public:
    virtual const char *name() const { return "volReverse"; }
    ControllerRemapVolReverse(): ControllerRemap(
        Midi::continuousController16,
        Midi::mainVolume,
        0, 127, 127, 0) { };
};

#endif
