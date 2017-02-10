#include <SDL2/SDL.h>
#include <iostream>

int main()
{
    SDL_Init( SDL_INIT_EVERYTHING );

    // Set postion and size for main window
    int mainSizeX = 600;
    int mainSizeY = 200;
    int mainPosX = 100;
    int mainPosY = 100;

    // Set up main window
    SDL_Window* mainWindow = SDL_CreateWindow( "Main Window", mainPosX, mainPosY, mainSizeX, mainSizeY, 0 );
    SDL_Renderer* mainRenderer = SDL_CreateRenderer( mainWindow, -1, SDL_RENDERER_ACCELERATED );
    SDL_SetRenderDrawColor( mainRenderer , 0, 0, 0, 255 );

    // Render empty ( red ) background in mainWindow
    SDL_RenderClear( mainRenderer );
    SDL_RenderPresent( mainRenderer );

    std::cin.ignore();
}
