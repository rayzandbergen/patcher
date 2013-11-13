/*! \file persistent.h
 *  \brief Contains functions to store persistent data in shared memory.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef PERSISTENT_H
#define PERSISTENT_H

class MemoryMap;

//! \brief This class uses shared memory to store information persistently between patcher runs.
class Persist
{
    MemoryMap *m_memMap;
public:
    Persist();
    void store(int track, int section);
    void restore(int *track, int *section);
};

#endif // PERSISTENT_H
