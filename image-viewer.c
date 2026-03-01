#include <SDL3/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {

    if (argc != 2) {
        printf("Error: Invalid arguments; expected file-path "
               "'path/to/image\n");

        return 0;
    }

    char *path = argv[1];
    char buffer[20000] = {0};
    FILE *file = fopen(path, "r");

    // printf("%ld", fread(&buffer, sizeof(char), 2, file));
    uint8_t chunk_size = 4;
    int counter = 0;
    while (fread(&buffer[counter], sizeof(char), chunk_size, file) != 0) {
        printf("%s\n", buffer);
        if (!memcmp(buffer, "IHDR", chunk_size)) {
            printf("IHDR SEEN");
        }
    }

    fclose(file);

    SDL_Window *window = SDL_CreateWindow("Simple Image Viewer", 900, 600, 0);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED);
    SDL_CreateRenderer(window, NULL);

    int x;
    int y;

    const SDL_FColor color = {0, 255, 0, 0};
    const SDL_FRect rect = {(float)x / 2, y, 90, 60};

    // if (window) {
    //
    //     SDL_Event event;
    //     bool isRunning = true;
    //
    //     // App loop
    //     while (isRunning) {
    //         // printf("Is running: %d\n", isRunning);
    //
    //         // Event handling
    //         while (SDL_PollEvent(&event)) {
    //
    //             switch (event.type) {
    //             case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    //                 isRunning = false;
    //                 break;
    //             case SDL_EVENT_KEY_DOWN:
    //                 isRunning = !(event.key.scancode == 20);
    //                 break;
    //             }
    //         }
    //
    //         // Rendering pipeline
    //         SDL_Renderer *renderer = SDL_GetRenderer(window);
    //
    //         SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b,
    //                                color.a);
    //         SDL_RenderClear(renderer);
    //         SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    //         //
    //         // Do pixel rendering here.
    //         //
    //         SDL_RenderPresent(renderer); // Display
    //
    //         // TODO: Explore render destructor and other safety/ semantic
    //         // practies defined in docs!!
    //
    //         // isRunning = false;
    //     }
    //
    //     return 0;
    // }
    //
    // printf("Error opening window.\n");
}
