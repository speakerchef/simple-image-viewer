#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char **argv) {

  int x = 900;
  int y = 600;
  const char *ALPHABETS[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I",
                             "J", "K", "L", "M", "N", "O", "P", "Q", "R",
                             "S", "T", "U", "V", "W", "X", "Y", "Z"};

  SDL_Window *window = SDL_CreateWindow("Simple Image Viewer", x, y, 0);
  SDL_Scancode scancodes;

  if (window) {

    SDL_Event event;
    while (true) {
      while (SDL_PollEvent(&event)) {

        switch (event.type) {

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          return 0;
        case SDL_EVENT_KEY_DOWN: {
          if (30 <= event.key.scancode && event.key.scancode < 39) {
            int key = event.key.scancode - 29;
            printf("%d\n", key);
          } else if (event.key.scancode == 39) {
            printf("%d\n", 0);
          } else if (event.key.scancode == 20) {
            return 0;
          }

          int length = sizeof(ALPHABETS) / sizeof(char *);
          for (int i = 0; i < length; i++) {
            if (event.key.scancode == i + 4)
              printf("%c\n", *ALPHABETS[i]);
          }
        }
        }
      }
    }
  }
  return 0;
}
