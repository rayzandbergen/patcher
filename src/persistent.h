/*! \file persistent.h
 *  \brief Contains functions to store persistent data in shared memory.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef PERSISTENT_H
#define PERSISTENT_H

class MemoryMap;

//! \brief Uses shared memory to store information persistently between patcher runs.
class Persist
{
    MemoryMap *m_memMap;    //!< The \a MemoryMap that should be made persistent.
public:
    Persist();
    void store(int track, int section, int trackWithinSet);
    void restore(int *track, int *section, int *trackWithinSet) const;
};

#endif // PERSISTENT_H
