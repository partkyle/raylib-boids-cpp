#include "raylib.h"

#include "rlgl.h"
#include "raymath.h"

#include "game.h"

#include "tracy/Tracy.hpp"

#include <iostream>

bool running = true;

void ThreadProc(GameData *data) {
    while (running) {
        ThreadWorker(data);
    }
}

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
    /*
    std::thread t0(ThreadProc, &data);
    std::thread t1(ThreadProc, &data);
    std::thread t2(ThreadProc, &data);
    std::thread t3(ThreadProc, &data);
    std::thread t4(ThreadProc, &data);
    std::thread t5(ThreadProc, &data);
    std::thread t6(ThreadProc, &data);
    std::thread t7(ThreadProc, &data);
    */

    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateAndRender(data);

        FrameMark;
    }
    /*
    running = false;
    t0.join();
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    */
    

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    return 0;
}
