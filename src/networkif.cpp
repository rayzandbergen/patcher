#include "networkif.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
/* This function runs "ifconfig" and parses the output */
/* to show only network interfaces and their addresses */
std::string getNetworkInterfaceAddresses()
{
    /* use C-style streams for the pipe, then parse the
       output line by line using C++ std::strings */
    FILE *fp = popen("/sbin/ifconfig", "r");
    if (!fp)
    {
        return std::string("could not run ifconfig *** ");
    }
    char lineBuffer[100];
    std::string interface;
    std::string address;
    size_t nofInterfaces = 0;
    std::ostringstream deviceString;
    while (!feof(fp))
    {
        if (!fgets(lineBuffer, sizeof(lineBuffer), fp))
            break;
        std::string line(lineBuffer);
        /* if the line does not start with whitespace, 
           then it is an interface name */
        size_t ifLen = line.find_first_of(" \t\n");
        if (ifLen != 0)
        {
            interface = line.substr(0, ifLen);
        }
        /* if this is an address line that does not belong to "lo"
           then parse the address */
        size_t addrOffset = line.find("inet addr:");
        if (addrOffset != line.npos && interface.compare("lo"))
        {
            nofInterfaces++;
            size_t addrBegin = 
                line.find_first_of("0123456789", addrOffset);
            size_t addrEnd = 
                line.find_first_not_of("0123456789.", addrBegin);
            address = line.substr(addrBegin, addrEnd-addrBegin);
            deviceString << interface << ": " << address << " *** ";
        }
    }
    pclose(fp);
    return nofInterfaces > 0 ? 
              deviceString.str() 
            : std::string("no network *** ");
}

