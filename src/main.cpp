#include "raylib.h"

#include "rlgl.h"
#include "raymath.h"

#include "game.h"

#include "tracy/Tracy.hpp"

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main ()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera mouse zoom");


    // SetTargetFPS(180);                   // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    GameData data;

    srand(time_t(NULL));

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateAndRender(data);

        FrameMark;
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    return 0;
}
