#include <SDL3/SDL.h>
#include <stdio.h>

int main(int argc, char **argv) {

  int x = 900;
  int y = 600;
  const SDL_FColor color = {128.0, 0.0, 128.0, 255.0};
  const SDL_FRect rect = {(float)x / 2, (float)y / 2, 90.0, 60.0};

  const char *ALPHABETS[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I",
                             "J", "K", "L", "M", "N", "O", "P", "Q", "R",
                             "S", "T", "U", "V", "W", "X", "Y", "Z"};

  SDL_Window *window = SDL_CreateWindow("Simple Image Viewer", x, y, 0);
  SDL_CreateRenderer(window, NULL);

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

      // rendering pipeline

      SDL_Renderer *renderer = SDL_GetRenderer(window);
      SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
      // SDL_RenderClear(renderer);
      SDL_RenderRect(renderer, &rect);
      SDL_RenderPresent(renderer);
    }
  }
  return 0;
}
