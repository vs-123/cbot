#ifndef UTIL_H
#define UTIL_H

/* append to a vector */
/* vector should have these elements -- data, capacity, count */
#define DAPPEND(vector, i)                                                     \
   do                                                                          \
      {                                                                        \
         if (vector.count >= vector.capacity)                                  \
            {                                                                  \
               vector.capacity                                                 \
                   = (vector.capacity) ? vector.capacity * 2 : 256;            \
               vector.elems                                                    \
                   = realloc (vector.elems,                                    \
                              vector.capacity * sizeof (vector.elems[0]));     \
            }                                                                  \
         vector.elems[vector.count++] = i;                                     \
      }                                                                        \
   while (0);

void cbot_log(const char *fmt, ...);

#endif /* UTIL_H */
