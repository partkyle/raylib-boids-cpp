#pragma once

#include <entt/entt.hpp>

#include "raylib.h"

#include "tracy/Tracy.hpp"

#include "entities.h"
#include "spatial_hash.h"
#include "config.h"

struct GameData {
    entt::registry reg;
    Camera2D camera;
    Config config;
    SpatialHash spatialHash;

    bool paused = false;

    GameData() : spatialHash(&config) {
        camera = {
            {},
            {},
            0,
            1,
        };

        config.bounds = { 100, 100, 1280 - 200, 720 - 200 };

        config.count = 1200;

        config.minSpeed = 200.0f;
        config.maxSpeed = 1000.0f;
        config.turnFactor = 10;

        config.avoidRadius = 40.0f;
        config.avoidFactor = 0.05f;

        config.visibleRadius = 100.0f;
        config.alignFactor = 0.05f;
        config.cohesionFactor = 0.0005f;

        config.cellSize = config.visibleRadius;
    };
};


int Init(GameData &data);
int UpdateAndRender(GameData &data);