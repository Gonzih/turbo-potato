#include <SDL.h>
#include <stdio.h>

#include <iostream>
#include <ncurses.h>
#include <locale.h>

#include "random.hpp"
/* #include "game.hpp" */
#include "window.hpp"
#include "logging.hpp"
#include "sig.hpp"

using namespace std;

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

int main() {
    logger::init("turbo-potato.log");
    sigint_handler_init();
    rand_init();

    //Initialize SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(strcat(strdup("SDL could not initialize! SDL_Error: "), SDL_GetError()));
    }

    {
        Window window(SCREEN_WIDTH, SCREEN_HEIGHT);

        SDL_Event e;

        bool quit = false;

        while (!quit) {
            while (SDL_PollEvent(&e) != 0) {
                if (e.type == SDL_QUIT)
                    quit = true;
            }

            window.render();
        }
    }

    SDL_Quit();

    return 0;
}
