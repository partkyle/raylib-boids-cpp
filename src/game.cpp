#include <iostream>

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "tracy/Tracy.hpp"

#include "game.h"

int Init(GameData &data)
{
    data.camera.zoom = 1.0;

    data.config.bounds = {100, 100, 1280 - 200, 720 - 200};

    data.config.count = 1200;

    data.config.minSpeed = 200.0f;
    data.config.maxSpeed = 1000.0f;
    data.config.turnFactor = 10;

    data.config.avoidRadius = 40.0f;
    data.config.avoidFactor = 0.05f;

    data.config.visibleRadius = 100.0f;
    data.config.alignFactor = 0.05f;
    data.config.cohesionFactor = 0.0005f;


    data.config.cellSize = data.config.visibleRadius;

    return 0;
}

float randf()
{
    return rand() / float(RAND_MAX);
}

float lerp(float lo, float hi, float amount)
{
    return amount * (hi - lo) + lo;
}

float randf_range(float lo, float hi)
{
    return lerp(lo, hi, randf());
}

void spawnBoids(entt::registry &reg, const Config &config, SpatialHash &spatialHash)
{
    ZoneScoped;

    auto boids = reg.view<const Boid>();

    if (boids.size() < config.count) {
        for (int i = 0; config.count - boids.size(); i++) {
            const auto entity = reg.create();
            reg.emplace<Boid>(entity);
            auto p = Vector2{ randf_range(config.bounds.x, config.bounds.width + config.bounds.x), randf_range(config.bounds.y, config.bounds.height + config.bounds.y) };
            reg.emplace<Position>(entity, p);
            reg.emplace<LastPosition>(entity, p);
            reg.emplace<Velocity>(entity, Vector2{randf_range(-config.maxSpeed, config.maxSpeed), randf_range(-config.maxSpeed, config.maxSpeed)});
            reg.emplace<BoidColor>(entity, Color{0, 255, 255, 255});

            auto [position, velocity, lastPosition] = reg.get<Position, Velocity, LastPosition>(entity);
            insert_into(spatialHash, config, entity, position, velocity, lastPosition, true);
        }
    }
    else if (boids.size() > config.count) {
        int toRemove = boids.size() - config.count;
        std::vector<entt::entity> entities;
        for (auto [entity] : boids.each()) {
            remove_from(spatialHash, config, entity);
            entities.push_back(entity);
            toRemove--;
            if (toRemove <= 0) break;
        }

        reg.destroy(entities.begin(), entities.end());
    }
}

void updateTurnFactor(entt::registry &reg, Config &config)
{
    ZoneScoped;
    
    const auto boids = reg.view<Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        if (position.p.x < config.bounds.x) {
            velocity.v.x += config.turnFactor;
        } else if (position.p.x > config.bounds.width + config.bounds.x) {
            velocity.v.x -= config.turnFactor;
        }

        if (position.p.y < config.bounds.y) {
            velocity.v.y += config.turnFactor;
        } else if (position.p.y > config.bounds.height + config.bounds.y) {
            velocity.v.y -= config.turnFactor;
        }
    }
}

void moveEntities(entt::registry &reg, float deltaTime)
{
    ZoneScoped;
    
    const auto boids = reg.view<Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        position.p = Vector2Add(position.p, Vector2Multiply(velocity.v, Vector2{deltaTime, deltaTime}));
    }
}

void boidLogic(entt::registry& reg, Config& config, const SpatialHash& spatialHash)
{
    ZoneScoped;

    const auto boids = reg.view<Boid, Position, Velocity>();

    reg.clear<Neighbor>();

    for (auto [entity, position, velocity] : boids.each()) {
        auto cell = positionToCell(position, config.cellSize);
        int neighborCount = 0;
        Vector2 close = {};
        Vector2 avgVelocity = {};
        Vector2 avgPosition = {};
        for (auto &[otherEntity] : get_all_in_cell(spatialHash, config, cell.first, cell.second)) {
            if (entity == otherEntity) continue;

            auto [otherPosition, otherVelocity] = reg.get<Position, Velocity>(otherEntity);

            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.avoidRadius) {
                close = Vector2Add(close, distance);
            }

            if (Vector2Length(distance) <= config.visibleRadius) {
                neighborCount++;
                avgVelocity = Vector2Add(avgVelocity, otherVelocity.v);
                avgPosition = Vector2Add(avgPosition, otherPosition.p);

                if (reg.all_of<Selected>(entity)) {
                    reg.emplace<Neighbor>(otherEntity);
                }
            }
        }

        velocity.v = Vector2Add(velocity.v, Vector2Multiply(close, Vector2{ config.avoidFactor, config.avoidFactor }));

        if (neighborCount > 0) {
            avgVelocity = Vector2Divide(avgVelocity, Vector2{ float(neighborCount), float(neighborCount) });
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Subtract(avgVelocity, velocity.v), Vector2{ config.alignFactor, config.alignFactor }));

            avgPosition = Vector2Divide(avgPosition, Vector2{ float(neighborCount), float(neighborCount) });
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Subtract(avgPosition, position.p), Vector2{ config.cohesionFactor, config.cohesionFactor }));
        }
    }
}

void mustGoFaster(entt::registry &reg, Config &config, float delta)
{
    ZoneScoped;
    
    const auto boids = reg.view<Boid, Velocity>();
    for (auto [entity, velocity] : boids.each()) {
        if (Vector2Length(velocity.v) < config.maxSpeed) {
            velocity.v = Vector2Lerp(velocity.v, Vector2Multiply(Vector2Normalize(velocity.v), Vector2{ config.maxSpeed, config.maxSpeed }), delta);
        }

        if (Vector2Length(velocity.v) > config.maxSpeed) {
            velocity.v = Vector2ClampValue(velocity.v, config.minSpeed, config.maxSpeed);
        }
    }
}

void drawBoids(const entt::registry &reg, const Config &config)
{
    ZoneScoped;

    const auto boids = reg.view<Boid, Position, Velocity, BoidColor>();

    for (auto [entity, position, velocity, color] : boids.each()) {
        
        auto x = (unsigned int)floor((velocity.v.x * 0.5 + config.maxSpeed) / (config.maxSpeed * 2) * 255);
        auto y = (unsigned int)floor((velocity.v.y * 0.5 + config.maxSpeed) / (config.maxSpeed * 2) * 255);

        Color c = Color{ (unsigned char)x, (unsigned char)y, 255, 255 };
        if (reg.all_of<Candidate>(entity)) {
            c = GREEN;
        }
        if (reg.all_of<Neighbor>(entity)) {
            c = RED;
        }
        if (reg.all_of<Selected>(entity)) {
            c = BLUE;
        }

        if (IsKeyDown(KEY_SPACE)) {
            auto size = Vector2{ 10, 10 };
            auto halfSize = Vector2{ size.x / 2.0f, size.y / 2.0f };
            DrawRectangleV(Vector2Subtract(position.p, halfSize), Vector2{ 10, 10 }, c);
        }
        else {
            float rot = atan2f(velocity.v.y, velocity.v.x) + PI / 2;
            float size = 10;
            auto v1 = Vector2Add(position.p, Vector2Rotate({ 0, -size * 2.0f }, rot));
            auto v2 = Vector2Add(position.p, Vector2Rotate({ -size * .8f, size }, rot));
            auto v3 = Vector2Add(position.p, Vector2Rotate({ size * .8f, size }, rot));
            DrawTriangle(v1, v2, v3, c);
        }
    }
}

void markCandidates(entt::registry &reg, const Config &config, const SpatialHash &spatialHash)
{
    ZoneScoped;

    reg.clear<Candidate>();

    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        auto cell = positionToCell(position, config.cellSize);
        for (auto &entry : get_all_in_cell(spatialHash, config, cell.first, cell.second)) {
            if (entry.entity != entity) {
                reg.emplace<Candidate>(entry.entity);
            }
        }
    }
}

void drawSpatialHashGrid(const entt::registry &reg, const Config &config)
{
    ZoneScoped;

    auto selected = reg.view<Position, Selected>();
    for ( auto [entity, position] : selected.each() ) {
        auto cell = positionToCell(position, config.cellSize);
        int radius = getSpatialRadius(config);
        for (int y = cell.second - radius; y <= cell.second + radius; y++) {
            for (int x = cell.first - radius; x <= cell.first + radius; x ++) {
                DrawRectangleLines(int(floorf(x * config.cellSize)), int(floorf(y * config.cellSize)), int(config.cellSize), int(config.cellSize), RED);
            }
        }
    }

}

void drawDebugLines(const entt::registry &reg, const Config &config)
{
    ZoneScoped;

    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        DrawCircleLines(position.p.x, position.p.y, config.avoidRadius, RED);
        DrawCircleLines(position.p.x, position.p.y, config.visibleRadius, YELLOW);
    }
}

void selectBoid(entt::registry &reg, const Config &config, const Camera2D &camera, const SpatialHash &spatialHash)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        reg.clear<Selected>();
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

        auto cell = positionToCell({mouse}, config.cellSize);

        if (!IsKeyDown(KEY_LEFT_SHIFT)) {
            reg.clear<Selected>();
        }

        entt::entity minEntity = {};
        float minDistance = FLT_MAX;
        for (auto &[entity] : get_all_in_cell(spatialHash, config, cell.first, cell.second)) {
            auto position = reg.get<Position>(entity);

            auto distance = Vector2Distance(position.p, mouse);
            if (distance < minDistance) {
                minDistance = distance;
                minEntity = entity;
            }
        }

        if (reg.valid(minEntity)) {
            reg.emplace<Selected>(minEntity);
        }
    }
}

void drawBounds(const Config& config)
{
    ZoneScoped;

    DrawRectangleLines(config.bounds.x, config.bounds.y, config.bounds.width, config.bounds.height, RED);
}

void updateBounds(Config &config, Camera2D &camera)
{
    int padding = 100;
    config.bounds.x = padding;
    config.bounds.y = padding;
    config.bounds.width = GetScreenWidth() - padding * 2;
    config.bounds.height = GetScreenHeight() - padding * 2;


    camera.target = { config.bounds.width / 2.0f + padding, config.bounds.height / 2.0f + padding };
    camera.offset = { config.bounds.width / 2.0f + padding, config.bounds.height / 2.0f + padding };
}

void updateZoom(Camera2D &camera)
{
    int mouseWheel = GetMouseWheelMove();
    if (mouseWheel != 0) {
        float zoomAmount = 0.05;
        float zoomMin = 0.01;
        camera.zoom += zoomAmount * mouseWheel;
        if (camera.zoom < zoomMin) camera.zoom = zoomMin;
    }

    if (IsKeyPressed(KEY_EQUAL)) {
        camera.zoom = 1;
    }
}

void drawDebugSelectedText(const entt::registry &reg, int startX, int startY, int fontSize)
{
    char buf[120];

    auto selected = reg.view<Selected, Position>();
    Vector2 selectedPos = {};
    for (auto [entity, position] : selected.each()) {
        sprintf_s(buf, "selected: %d (%0.2f, %0.2f)", entity, position.p.x, position.p.y);
        DrawText(buf, startX, startY, fontSize, Color{ 0, 255, 255, 255 });
        startY += fontSize;
        selectedPos = position.p;
    }

    auto candidates = reg.view<Candidate, Position>();

    std::vector<std::tuple<entt::entity, Position, float>> positions;

    for (auto [entity, position] : candidates.each()) {
        positions.push_back(std::tuple(entity, position, Vector2Distance(selectedPos, position.p)));
    }

    std::sort(positions.begin(), positions.end(), [](auto a, auto b) {
        return std::get<2>(a) < std::get<2>(b); 
    });

    for (auto [entity, position, distance] : positions) {
        sprintf_s(buf, "%d (%0.2f, %0.2f) distance: %0.2f", entity, position.p.x, position.p.y, distance);

        auto color = GREEN;
        if (reg.all_of<Neighbor>(entity)) color = RED;
        DrawText(buf, startX + 10, startY, fontSize, color);

        startY += fontSize;
    }
}

void draw(const GameData &data, const Config & config)
{
    ZoneScoped;

    Camera2D textCamera = {};
    textCamera.zoom = 1.0;

    BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(data.camera);
        drawBoids(data.reg, config);
        drawSpatialHashGrid(data.reg, data.config);
        drawDebugLines(data.reg, data.config);
        drawBounds(data.config);
        EndMode2D();

        BeginMode2D(textCamera);
        char buf[80];
        sprintf_s(buf, "boid count: %d", data.config.count);
        DrawText(buf, 10, 10, 20, Color{ 0, 255, 255, 255 });

        sprintf_s(buf, "fps: %d", GetFPS());
        DrawText(buf, 10, 30, 20, Color{ 0, 255, 255, 255 });

        int start = 50;
        int fontSize = 20;

        // drawDebugSelectedText(data.reg, 10, start, fontSize);
        EndMode2D();
    EndDrawing();
}

void updateSpatialHash(GameData & data)
{
    ZoneScoped;

    auto boids = data.reg.view<const Boid, const Position, const Velocity, LastPosition>();

    for (auto [entity, p, v, l] : boids.each()) {
        insert_into(data.spatialHash, data.config, entity, p, v, l);
        // update the last position now that we've update the spatial
        // so it can be used next frame
        l.p = p.p;
    }
}

void updatePause(GameData& data)
{
    if (IsKeyPressed(KEY_SPACE)) {
        data.paused = !data.paused;
    }
}

int UpdateAndRender(GameData & data)
{
    ZoneScoped;

    float delta = GetFrameTime();

    updateBounds(data.config, data.camera);
    updateZoom(data.camera);
    updatePause(data);

    spawnBoids(data.reg, data.config, data.spatialHash);

    updateSpatialHash(data);
    selectBoid(data.reg, data.config, data.camera, data.spatialHash);
    markCandidates(data.reg, data.config, data.spatialHash);

    if (!data.paused) {
        boidLogic(data.reg, data.config, data.spatialHash);
        updateTurnFactor(data.reg, data.config);
        mustGoFaster(data.reg, data.config, delta);
        moveEntities(data.reg, delta);
    }

    // Draw
    //----------------------------------------------------------------------------------
    draw(data, data.config);
    //----------------------------------------------------------------------------------

    return 0;
}
