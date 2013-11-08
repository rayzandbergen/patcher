#include "persistent.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>

class MemoryMap
{
public:
    // update this if layout changes
    static const int expectMagic = 0xafbe8219;
    int m_magic;
    bool m_valid;
    int m_track;
    int m_section;
};

Persist::Persist(): m_memMap(0)
{
    bool error = true;
    do
    {
        int fd = shm_open("/patcher-persistent",
                O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
        if (fd == -1)
        {
            perror("shm_open");
            break;
        }
        struct stat statBuf;
        if (-1 == fstat(fd, &statBuf))
        {
            perror("fstat");
            break;
        }
        bool resized = false;
        if (statBuf.st_size != sizeof(MemoryMap))
        {
            if (-1 == ftruncate(fd, (off_t)sizeof(MemoryMap)))
            {
                perror("fstat");
                break;
            }
            resized = true;
        }
        m_memMap = (MemoryMap*)mmap(0, sizeof(MemoryMap),
                            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if (m_memMap == 0)
        {
            perror("mmap");
            break;
        }
        if (resized ||
            m_memMap->m_magic != MemoryMap::expectMagic ||
            !m_memMap->m_valid)
        {
            store(0, 0);
        }
        error = false;
    } while(0);
    if (error)
    {
        throw(1);
    }
}

void Persist::store(int track, int section)
{
    m_memMap->m_valid = false;
    m_memMap->m_magic = MemoryMap::expectMagic;
    m_memMap->m_track = track;
    m_memMap->m_section = section;
    m_memMap->m_valid = true;
}

void Persist::restore(int *track, int *section)
{
    *track = m_memMap->m_track;
    *section = m_memMap->m_section;
}

#if 0
int main(void)
{
    Persist p;
    //p.store(3,4);
    int t,s;
    p.restore(&t, &s);
    printf("%d %d\n", t, s);
}
#endif
