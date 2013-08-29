#include <errno.h>
#include <string.h>
class Error
{
public:
    char m_message[1024];
    Error(const char *s)
    {
        strcpy(m_message, s);
    }
    Error(const char *s, int errNo)
    {
        strcpy(m_message, s);
        strcat(m_message, ": ");
        strcat(m_message, strerror(errNo));
    }
};
