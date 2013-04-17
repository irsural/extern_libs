
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
#endif // defined (__ICCARM__)

