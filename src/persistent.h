#ifndef PERSISTENT_H
#define PERSISTENT_H

class MemoryMap; // actual shared memory layout

// This class uses shared memory to store
// information persistently between patcher runs.
class Persist
{
    MemoryMap *m_memMap;
public:
    Persist();
    void store(int track, int section);
    void restore(int *track, int *section);
};

#endif // PERSISTENT_H
