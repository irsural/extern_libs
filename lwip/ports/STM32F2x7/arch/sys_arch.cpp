
extern "C" {
#include <lwip/sys.h>
}

#include <irstime.h>

u32_t sys_now()
{
  const double seconds = CNT_TO_DBLTIME(counter_get());
  return static_cast<u32_t>(seconds*1000);
}
