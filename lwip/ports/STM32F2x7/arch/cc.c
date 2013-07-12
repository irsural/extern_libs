
#include <arch/cc.h>

#if defined (__ICCARM__)
u16_t htons_arch(u16_t n)
{
  return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

u32_t htonl_arch(u32_t n)
{
  return ((n & 0xff) << 24) |
    ((n & 0xff00) << 8) |
    ((n & 0xff0000UL) >> 8) |
    ((n & 0xff000000UL) >> 24);
}

lwip_accept_udp_port_fun_t lwip_accept_udp_port_fun;

void set_lwip_accept_udp_port_fun(lwip_accept_udp_port_fun_t ap_fun)
{
  lwip_accept_udp_port_fun = ap_fun;
}

int lwip_accept_udp_port(u16_t a_port)
{
  if (lwip_accept_udp_port_fun) {
    return lwip_accept_udp_port_fun(a_port);
  } else {
    return 0;
  }
}
#endif // defined (__ICCARM__)

