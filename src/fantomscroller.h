#ifndef FANTOM_SCREEN_SCROLLER_H
#define FANTOM_SCREEN_SCROLLER_H

#include <string>
#include "fantom.h"
#include "timestamp.h"

//! \brief Show the current network interface on the Fantom screen.
class FantomScreenScroller {
    bool m_enabled;                                 //!< Enable switch.
    Fantom::Driver *m_fantom;                       //!< Fantom driver.
    TimeSpec m_lastUpdateTime;                      //!< Previous time the Fantom display was updated.
    size_t m_offset;                                //!< Scroll offset.
    std::string m_text;                             //!< Info text to be displayed.
public:
    //! \brief Query on/off status.
    bool enabled() const { return m_enabled; }
    void update(TimeSpec &now);
    void toggle();
    FantomScreenScroller(Fantom::Driver *fantomDriver);
};
#endif // FANTOM_SCREEN_SCROLLER_H
