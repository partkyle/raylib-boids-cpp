#pragma once

#include <entt/entt.hpp>

#include "raylib.h"

struct Config {
    int count;

    Rectangle bounds;

    float cellSize;

    float minSpeed;
    float maxSpeed;

    float turnFactor;

    float avoidRadius;
    float avoidFactor;
    
    float visibleRadius;
    float alignFactor;
    float cohesionFactor;
};

struct GameData {
    entt::registry reg;

    Camera2D camera;

    Config config;
};


int Init(GameData &data);
int UpdateAndRender(GameData &data);