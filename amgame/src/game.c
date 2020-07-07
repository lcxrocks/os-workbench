#include <game.h>
#include <klib.h>
#include <assert.h>
#define FPS 30
#define N 32
#define v 10
/* use the following API */
// uint32_t uptime();
// void get_timeofday(void *rtc);
// int read_key();
// void draw_rect(uint32_t *pixels, int x, int y, int w, int h);
// void draw_sync();
// int screen_width();
// int screen_height();

// Operating system is a C program!
#define KEYNAME(key) \
    [_KEY_##key] = #key,
static const char *key_names[] = {
    _KEYS(KEYNAME)
  };

uint32_t color_buf[N*N];
int x, y, w, h;
int flag;
void check(){
  if((x == w/2) && (y == h/2) && (flag == 1)){
    black();
  }
}

void update(int key){
  printf("key: %s\n", key_names[key]);
  switch (key)
  {
  case _KEY_UP:
     y = y - v;
     flag = 1;
    break;
  case _KEY_DOWN:
     y = y + v;
     flag = 1;
    break;
  case _KEY_LEFT:
     x = x - v;
     flag = 1;
    break;
  case _KEY_RIGHT:
     x = x + v;
     flag = 1;
     break;
  default:
     x = x; y = y;
     printf("x: %d, y = %d\n",x,y);
    break;
  }
  check();
}

void redraw(){
  //printf("w: %d, h: %d\n", w,h);
  memset(color_buf, 0xff,sizeof(color_buf));
  draw_rect(color_buf, x, y, N, N);
  draw_sync();
}

int main(const char *args) {
  _ioe_init();

  unsigned long last = 0;
  unsigned long fps_last = 0;
  int fps = 0;
  int key;
  w = screen_width();
  h = screen_height();
  x = w/2;
  y = h/2;
  // puts("mainargs = \"");
  // puts(args); // make run mainargs=xxx
  // puts("\"\n");
  //video_test();
  
  while (1) {
    unsigned long upt = uptime();
    key = read_key();
    if (key!=_KEY_NONE)
    {
      //printf("getkey!\n");
      if(key == _KEY_ESCAPE) _halt(1);
      if (upt - last > 1000 / FPS) {
        update(key); //update_game_process
        redraw(); //redraw_window
        last = upt;
        fps ++;
      }

      if (upt - fps_last > 1000) {
        // display fps every 1s
        printf("%d: FPS = %d\n", upt, fps);
        fps_last = upt;
        fps = 0;
      }
    }
  }
  return 0;
}
