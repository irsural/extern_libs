#ifndef IRSCERRORH
#define IRSCERRORH

void throw_error(const char* ap_file_name, int a_line_number,
  const char* ap_msg);

void printf_mlog(const char *format, ...);

#endif //IRSCERRORH
