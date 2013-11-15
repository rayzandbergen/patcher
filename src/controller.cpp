/*! \file controller.cpp
 *  \brief Contains an object remap MIDI Controller messages.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "controller.h"

//! \brief Interpolate controller value.
Real ControllerRemap::interpolate(Real x, Real x0, Real y0, Real x1, Real y1) const
{
    Real f = (x - x0)/(x1-x0);
    if (f < 0)
        return y0;
    if (f > (Real)1.0)
        return y1;
    return y0 + f*(y1-y0);
}

//! \brief Do the actual controller remapping.
void ControllerRemap::value(uint8_t controller, uint8_t val, uint8_t *controllerOut, uint8_t *valOut) const
{
    if (controller != m_from)
    {
        *controllerOut = controller;
        *valOut = val;
        return;
    }
    *controllerOut = m_to;
    *valOut = (uint8_t)interpolate((Real)val, m_x0, m_y0, m_x1, m_y1);
}

Real ControllerRemapVolQuadratic::interpolate(Real x,
    Real x0, Real y0, Real x1, Real y1) const
{
    Real f = (x - x0)/(x1-x0);
    f = (Real)1.0 - f;
    f *= f;
    f = (Real)1.0 - f;
    //f = (Real)1-(f-(Real)1)*(f-(Real)1);
    if (f < 0)
        return y0;
    if (f > (Real)1.0)
        return y1;
    return y0 + f*(y1-y0);
}

