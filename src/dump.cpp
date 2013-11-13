/*! \file dump.cpp
 *  \brief Contains an object to make binary dumps of other objects.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */
#include <stdlib.h>
#include <string.h>
#include "dump.h"
void Dump::save(const char *s) const
{
    for (int i=0;;i++)
    {
        write(m_fd, s+i, 1);
        if (s[i] == 0)
            break;
    }
}
void Dump::restore(char *s) const
{
    for (int i=0;;i++)
    {
        read(m_fd, s+i, 1);
        if (s[i] == 0)
            break;
    }
}
void Dump::restore(char **s) const
{
    char buf[200];
    int i;
    for (i=0;;i++)
    {
        read(m_fd, buf+i, 1);
        if (buf[i] == 0)
            break;
    }
    int n=i+1;
    *s = (char*)malloc(n);
    memcpy(*s, buf, n);
}
bool Dump::fopen(const char *fileName, int flags, int mode)
{
    m_fd = open(fileName, flags, mode);
    return m_fd >= 0;
}
void Dump::fclose(void)
{
    close(m_fd);
}
