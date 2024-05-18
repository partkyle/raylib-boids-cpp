#include "game.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

int UpdateAndRender(GameData * data) {

    int *zoomMode = &data->zoomMode;
    if (IsKeyPressed(KEY_ONE)) *zoomMode = 0;
    else if (IsKeyPressed(KEY_TWO)) *zoomMode = 1;
    
    // Translate based on mouse right click
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f/data->camera.zoom);
        data->camera.target = Vector2Add(data->camera.target, delta);
    }

    if (*zoomMode == 0)
    {
        // Zoom based on mouse wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            // Get the world point that is under the mouse
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), data->camera);

            // Set the offset to where the mouse is
            data->camera.offset = GetMousePosition();

            // Set the target to match, so that the camera maps the world space point 
            // under the cursor to the screen space point under the cursor at any zoom
            data->camera.target = mouseWorldPos;

            // Zoom increment
            float scaleFactor = 1.0f + (0.25f*fabsf(wheel));
            if (wheel < 0) scaleFactor = 1.0f/scaleFactor;
            data->camera.zoom = Clamp(data->camera.zoom*scaleFactor, 0.125f, 64.0f);
        }
    }
    else
    {
        // Zoom based on mouse left click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Get the world point that is under the mouse
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), data->camera);

            // Set the offset to where the mouse is
            data->camera.offset = GetMousePosition();

            // Set the target to match, so that the camera maps the world space point 
            // under the cursor to the screen space point under the cursor at any zoom
            data->camera.target = mouseWorldPos;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            // Zoom increment
            float deltaX = GetMouseDelta().x;
            float scaleFactor = 1.0f + (0.01f*fabsf(deltaX));
            if (deltaX < 0) scaleFactor = 1.0f/scaleFactor;
            data->camera.zoom = Clamp(data->camera.zoom*scaleFactor, 0.125f, 64.0f);
        }
    }
    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(data->camera);

            // Draw the 3d grid, rotated 90 degrees and centered around 0,0 
            // just so we have something in the XY plane
            rlPushMatrix();
                rlTranslatef(0, 25*50, 0);
                rlRotatef(90, 1, 0, 0);
                DrawGrid(100, 50);
            rlPopMatrix();

            // Draw a reference circle
            DrawCircle(GetScreenWidth()/2, GetScreenHeight()/2, 50, MAROON);
            
        EndMode2D();

        DrawText("[1][2] Select mouse zoom mode (Wheel or Move)", 20, 20, 20, DARKGRAY);
        if (*zoomMode == 0) DrawText("Mouse right button drag to move, mouse wheel to zoom", 20, 50, 20, DARKGRAY);
        else DrawText("Mouse right button drag to move, mouse press and move to zoom", 20, 50, 20, DARKGRAY);
    
    EndDrawing();
    //----------------------------------------------------------------------------------

    return 0;
}