#ifndef LAN_CONFIG_H
#define LAN_CONFIG_H

#include <stdint.h>

//#define WITH_ICMP // enable support for ping
//#define WITH_UDP  // sending/receiving UDP packages
//#define WITH_DHCP // DHCP IP lookup, instead of a static IP (requires WITH_UDP)
//#define WITH_TCP  // TCP connection support
//#define WITH_HTTPD  // minimal HTTPD webserver (requires WITH_TCP)
//#define WITH_NTP  // Enable a NTP client (requires WITH_UDP, WITH_DHCP is recommended, because it fills in the NTP server automatically)

#ifndef WITH_DHCP
#define IP_ADDR inet_addr(172, 16, 0, 10)
#define IP_SUBNET_MASK inet_addr(255, 255, 0, 0)
#define IP_DEFAULT_GATEWAY inet_addr(172, 16, 0, 1)
#endif


/***
 *  Compile-time check for macros
 ***/
#define COMPILE_CHECK(x)    \
  {                         \
    struct _CC {            \
      char a[(x) ? 1 : -1]; \
    };                      \
  }


#endif