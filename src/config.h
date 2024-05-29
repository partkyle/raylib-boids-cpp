#pragma once

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