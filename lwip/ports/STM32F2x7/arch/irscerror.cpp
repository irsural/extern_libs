#include <irserror.h>
#include <stdarg.h>

extern "C" {
# include "irscerror.h"
}

void throw_error(const char* ap_file_name, int a_line_number, const char* ap_msg)
{
  irs::error_trans()->throw_error(1, ap_file_name, a_line_number,
    irs::spec_assert("Assert", ap_msg));
}

void printf_mlog(const char *format, ...)
{
  const int buffer_size = 200;
  char buffer[buffer_size];
  va_list args;
  va_start (args, format);
  vsnprintf(buffer, buffer_size, format, args);
  va_end (args);
  irs::mlog() /*<< irs::sdatetime*/ << buffer;
}
