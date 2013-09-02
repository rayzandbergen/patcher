#ifndef SEQUENCER_H
#define SEQUENCER_H
#include <iostream>
#include <fstream>
class Sequencer
{
private:
    std::ofstream *m_fp;
    std::string fileName() const;
    bool m_enabled;
public:
    void enable();
    void disable();
    bool toggle();
    Sequencer(): m_enabled(false) { }
};
#endif //SEQUENCER_H
