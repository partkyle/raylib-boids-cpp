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

    bool paused;
};


int Init(GameData &data);
int UpdateAndRender(GameData &data);