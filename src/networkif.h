#ifndef NETWORKIF_H
#define NETWORKIF_H
#include <string>
/* This function runs "ifconfig" and parses the output */
/* to show only network interfaces and their addresses */
std::string getNetworkInterfaceAddresses();
#endif // NETWORKIF_H

