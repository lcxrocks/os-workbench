#include <am.h>
#include <amdev.h>

void splash();
void print_key();
static inline void puts(const char *s) {
  for (; *s; s++) _putc(*s);
}
<<<<<<< HEAD
void black();
=======
>>>>>>> d2975df19786aa7cfe393d99e2579d4f679cdbc7
