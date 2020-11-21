#include <SDL.h>
#include <stdio.h>
#include <iostream>
#include <locale.h>

#include "random.hpp"
/* #include "game.hpp" */
#include "game.hpp"
#include "sdl/sdl.hpp"
#include "logging.hpp"

using namespace std;

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 960;

int main()
{
    logger::init("turbo-potato.log");
    rand_init();

    sdl::init();
    atexit(SDL_Quit);

    Game game { SCREEN_WIDTH, SCREEN_HEIGHT};
    game.init();
    game.loop();

    return 0;
}
