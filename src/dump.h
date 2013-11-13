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
class Dump
{
    int m_fd;
    public:
    template <class T> void save(T &x) const
    {
        write(m_fd, (const void*)&x, sizeof(x));
    }
    template <class T> void restore(T &x) const
    {
        read(m_fd, (void*)&x, sizeof(x));
    }
    void save(const char *s) const;
    void restore(char *s) const;
    void restore(char **s) const;
    bool fopen(const char *fileName, int flags, int mode);
    void fclose(void);
};
#endif
