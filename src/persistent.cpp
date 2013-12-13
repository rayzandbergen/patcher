/*! \file persistent.cpp
 *  \brief Contains functions to store persistent data in shared memory.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include "persistent.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "error.h"

//! \brief Memory map of persistent data: current \a Track and \a Section index.
class MemoryMap
{
public:
    static const int expectMagic = 0xafbe821a;  //!<    Magic number, must be changed if layout changes.
    int m_magic;                                //!<    Magic number.
    bool m_valid;                               //!<    Validity flag.
    int m_track;                                //!<    Current \a Track number.
    int m_section;                              //!<    Current \a Section number.
    int m_trackWithinSet;                       //!<    Track index within setlist.
};

/*! \brief Constructor.
 *
 * Creates some shared memory, or attaches to existing.
 */
Persist::Persist(): m_memMap(0)
{
    int fd = shm_open("/patcher-persistent",
            O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    if (fd == -1)
    {
        throw(Error("shm_open", errno));
    }
    struct stat statBuf;
    if (-1 == fstat(fd, &statBuf))
    {
        throw(Error("fstat", errno));
    }
    bool resized = false;
    if (statBuf.st_size != sizeof(MemoryMap))
    {
        if (-1 == ftruncate(fd, (off_t)sizeof(MemoryMap)))
        {
            throw(Error("ftruncate", errno));
        }
        resized = true;
    }
    m_memMap = (MemoryMap*)mmap(0, sizeof(MemoryMap),
                        PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (m_memMap == 0)
    {
        throw(Error("mmap", errno));
    }
    if (resized ||
        m_memMap->m_magic != MemoryMap::expectMagic ||
        !m_memMap->m_valid)
    {
        store(0, 0, 0); // clear
    }
}

/*! \brief  Store current \a Track, \a Section and set index in shared memory.
 */
void Persist::store(int track, int section, int trackWithinSet)
{
    m_memMap->m_valid = false;
    m_memMap->m_magic = MemoryMap::expectMagic;
    m_memMap->m_track = track;
    m_memMap->m_section = section;
    m_memMap->m_trackWithinSet = trackWithinSet;
    m_memMap->m_valid = true;
}

/*! \brief  Restore current \a Track, \a Section and set index from shared memory.
 */
void Persist::restore(int *track, int *section, int *trackWithinSet) const
{
    *track = m_memMap->m_track;
    *section = m_memMap->m_section;
    *trackWithinSet = m_memMap->m_trackWithinSet;
}

