#pragma once
#include <entt/entt.hpp>
#include "raylib.h"

struct GameData {
    entt::registry regitry;

    Camera2D camera;

    int zoomMode;
};

int UpdateAndRender(GameData *data);