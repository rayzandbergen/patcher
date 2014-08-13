/*! \file controller.h
 *  \brief Contains an object remap MIDI Controller messages.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef CONTROLLER_REMAP_H
#define CONTROLLER_REMAP_H
#include <stdint.h>
#include "patchercore.h"
#include "mididef.h"

//! \brief Namespace for controller remap objects.
namespace ControllerRemap {

/*! \brief Remaps a single controller to a new controller, with its value interpolated to a new value.
 *
 * This object by itself does not do anything. Its descendants provide
 * more interesting behaviour.
 */
class Default {
protected:
    uint8_t m_from;                 //!<    Controller to map from.
    uint8_t m_to;                   //!<    Controller to map to.
    Real m_x0;                      //!<    Interpolation 'from' value x0.
    Real m_y0;                      //!<    Interpolation 'to' value y0.
    Real m_x1;                      //!<    Interpolation 'from' value x1.
    Real m_y1;                      //!<    Interpolation 'to' value y1.
public:
    //! \brief The name of the controller, so the configuration can refer to it.
    virtual const char *name() const = 0;
    virtual bool value(uint8_t controller, uint8_t val,
        uint8_t *controllerOut, uint8_t *valOut) const;
    virtual Real interpolate(Real x, Real x0, Real y0,
        Real x1, Real y1) const;
    //! \brief Constructs a Default that does nothing.
    Default(
        uint8_t from = 255, uint8_t to = 255,
        uint8_t x0 = 0,     uint8_t y0 = 0,
        uint8_t x1 = 127,   uint8_t y1 = 127):
        m_from(from), m_to(to), m_x0(x0), m_y0(y0),
        m_x1(x1), m_y1(y1) { }
    virtual ~Default() { }
};

/*! \brief Drops continuous controller 16.
 */
class Drop16: public Default {
public:
    Drop16(): Default(
        Midi::continuousController16,
        Midi::continuousController16
    ) { }
    virtual const char *name() const { return "drop16"; }
    virtual bool value(uint8_t controller, uint8_t val,
        uint8_t *controllerOut, uint8_t *valOut) const;
};

/*! \brief Remaps continuous controller 16 to volume, and replaces its value curve with a parabola.
 */
class VolQuadratic: public Default {
public:
    virtual const char *name() const { return "volQuadratic"; }
    VolQuadratic(): Default(
        Midi::continuousController16,
        Midi::mainVolume
    ) { }
    virtual Real interpolate(Real x, Real x0, Real y0,
        Real x1, Real y1) const;
};

/*! \brief Remaps continuous controller 16 to volume, and replaces its value curve with an inverted parabola, used for crossfades with \a VolQuadratic.
 */
class VolReverse: public Default {
public:
    virtual const char *name() const { return "volReverse"; }
    VolReverse(): Default(
        Midi::continuousController16,
        Midi::mainVolume,
        0, 127, 127, 0) { };
};

} // namespace ControllerRemap

#endif
