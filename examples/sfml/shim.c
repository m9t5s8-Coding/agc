/*
 * Small C bridge for the SFML example.
 *
 * The CSFML API passes sfVideoMode and sfColor by value.  AGC's `extern "c"`
 * currently passes struct values as pointers, so calling those functions
 * directly would use the wrong ABI.  These wrappers take scalar arguments and
 * construct the structs on the C side.
 */

#include <CSFML/Graphics.h>
#include <CSFML/Window.h>

#include <stdbool.h>
#include <stdint.h>

void*
ag_sf_window_create(int width,
                    int height,
                    int bits_per_pixel,
                    const char* title,
                    int style,
                    int state) {
  sfVideoMode mode;
  mode.size.x       = (unsigned int)width;
  mode.size.y       = (unsigned int)height;
  mode.bitsPerPixel = (unsigned int)bits_per_pixel;

  sfRenderWindow* window = sfRenderWindow_create(
      mode, title, (uint32_t)style, (sfWindowState)state, NULL);
  if (window) {
    sfRenderWindow_setFramerateLimit(window, 60);
    sfRenderWindow_setVerticalSyncEnabled(window, true);
  }
  return window;
}

int
ag_sf_window_should_close(void* handle) {
  sfRenderWindow* window = (sfRenderWindow*)handle;
  sfEvent         event;

  while (sfRenderWindow_pollEvent(window, &event)) {
    if (event.type == sfEvtClosed)
      return 1;
    if (event.type == sfEvtKeyPressed && event.key.code == sfKeyEscape)
      return 1;
  }

  return sfRenderWindow_isOpen(window) ? 0 : 1;
}

void
ag_sf_window_clear(void* handle,
                   unsigned char red,
                   unsigned char green,
                   unsigned char blue,
                   unsigned char alpha) {
  sfColor color = {red, green, blue, alpha};
  sfRenderWindow_clear((sfRenderWindow*)handle, color);
}

void*
ag_sf_rect_create(int           width,
                  int           height,
                  unsigned char red,
                  unsigned char green,
                  unsigned char blue,
                  unsigned char alpha) {
  sfRectangleShape* shape = sfRectangleShape_create();
  sfVector2f        size  = {(float)width, (float)height};
  sfColor           color = {red, green, blue, alpha};
  sfRectangleShape_setSize(shape, size);
  sfRectangleShape_setFillColor(shape, color);
  return shape;
}

void
ag_sf_rect_set_position(void* handle, int x, int y) {
  sfVector2f position = {(float)x, (float)y};
  sfRectangleShape_setPosition((sfRectangleShape*)handle, position);
}


