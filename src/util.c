#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void
cbot_log (const char *format, ...)
{
   time_t now   = time (NULL);
   struct tm *t = localtime (&now);

   printf ("[CBOT_LOG %04d-%02d-%02d %02d:%02d:%02d] ", t->tm_year + 1900, t->tm_mon + 1,
           t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

   va_list args;
   va_start (args, format);
   vprintf (format, args);
   va_end (args);

   printf ("\n");
}
