/*! \file dump.h
 *  \brief Contains an object to make binary dumps of other objects.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#ifndef DUMP_H
#define DUMP_H
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "timer.h"
/*! \brief This object reads and writes binary dumps of other objects.
 *
 * The dumps are portable. Ints are written in network byte order, i.e.
 * big endian.
 */
class Dump
{
    int m_fd;       //!<    File descriptor for reading/writing.
public:
    //! \brief Write a binary image of an object.
    template <class T> void saveInt(T &x) const
    {
        for (size_t i = sizeof(x); i !=0; i--)
        {
            uint8_t b = (uint8_t)((x >> 8*(i-1)) & 0xff);
            g_timer.write(m_fd, (const void*)&b, 1);
        }
    }
    //! \brief restore a binary image of an object.
    template <class T> void restoreInt(T &x) const
    {
        T y = 0;
        for (size_t i = sizeof(x); i !=0; i--)
        {
            uint8_t b;
            g_timer.read(m_fd, (void*)&b, 1);
            y |= (T)b << 8*(i-1);
        }
        x = y;
    }
    //! \brief Write a null-terminated string.
    void save(const char *s) const;
    //! \brief Restore a null-terminated string.
    void restore(char *s) const;
    //! \brief Restore a null-terminated string, space is malloc'ed.
    void restore(char **s) const;
    //! \brief open(2) a file.
    bool fopen(const char *fileName, int flags, int mode);
    //! \brief close the associated file.
    void fclose();
};
#endif
