#define RED 31
#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define PURPLE 35
#define CYAN 36

#define panic_on(cond, s) \
  do { \
    if (cond) { \
      printf("%s", s); \
      printf(" Line:  %d\n", __LINE__); \
      return 1; \
    } \
  } while (0)

#define panic(s) panic_on(1, s)

#define c_panic_on(color, cond, s) \
do{ \
    if(cond) {\
        printf("\033[%dm", color); \
        panic_on(cond, s); \
        printf("\033[0m"); \
    }\
}while(0)

#define r_panic_on(cond, s) c_panic_on(RED, cond, s);

#define c_log(color, ...) \
    printf("\033[%dm", color); \
    printf(__VA_ARGS__); \
    printf("\033[0m"); 
    