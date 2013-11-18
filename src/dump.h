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
/*! \brief This object reads and writes binary dumps of other objects.
 */
class Dump
{
    int m_fd;       //!<    File descriptor for reading/writing.
public:
    //! \brief Write a binary image of an object.
    template <class T> void save(T &x) const
    {
        write(m_fd, (const void*)&x, sizeof(x));
    }
    //! \brief restore a binary image of an object.
    template <class T> void restore(T &x) const
    {
        read(m_fd, (void*)&x, sizeof(x));
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
