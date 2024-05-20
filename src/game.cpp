#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "game.h"

int Init(GameData &data)
{
    data.camera.zoom = 1.0;

    data.config.bounds = {100, 100, 1280 - 200, 720 - 200};

    data.config.cellSize = 10.0;

    data.config.count = 10;

    data.config.minSpeed = 2.0f;
    data.config.maxSpeed = 100.0f;
    data.config.turnFactor = 100.0;

    data.config.avoidRadius = 40.0f;
    data.config.avoidFactor = 0.05f;

    data.config.visibleRadius = 100.0f;
    data.config.alignFactor = 0.05f;
    data.config.cohesionFactor = 0.0005f;

    return 0;
}

struct Boid {};

struct Position
{
    Vector2 p;
};

struct Velocity
{
    Vector2 v;
};

struct BoidColor
{
    Color color;
};

struct Selected {};
struct Candidate {};
struct Neighbor {};

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


struct HashEntry {
    entt::entity entity;
    Position p;
    Velocity v;
};

std::pair<int, int> positionToCell(const Position &p, float cellSize)
{
    return std::pair(int(p.p.x / cellSize),  int(p.p.y / cellSize));
}

int hashCell(std::pair<int, int> p) {
    return p.first * 92837111 + p.second * 689287499;
}

void spawnBoids(entt::registry &reg, const Config &config)
{
    auto boids = reg.view<const Boid>();

        if (boids.size() < config.count) {
        for (int i = 0; i , config.count - boids.size(); i++) {
            const auto entity = reg.create();
            reg.emplace<Boid>(entity);
            reg.emplace<Position>(entity, Vector2{randf_range(config.bounds.x, config.bounds.width + config.bounds.x), randf_range(config.bounds.y, config.bounds.height + config.bounds.y)});
            reg.emplace<Velocity>(entity, Vector2{randf_range(-config.maxSpeed, config.maxSpeed), randf_range(-config.maxSpeed, config.maxSpeed)});
            reg.emplace<BoidColor>(entity, Color{0, 255, 255, 255});
        }
    }
}

void updateTurnFactor(entt::registry &reg, Config &config)
{
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
    const auto boids = reg.view<Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        position.p = Vector2Add(position.p, Vector2Multiply(velocity.v, Vector2{deltaTime, deltaTime}));
    }
}

void boidLogic(entt::registry& reg, Config& config, const std::unordered_map<int, std::vector<HashEntry>>& spatialHash)
{
    const auto boids = reg.view<Boid, Position, Velocity>();

    reg.clear<Neighbor>();

    for (auto [entity, position, velocity] : boids.each()) {
        int h = hashCell(positionToCell(position, config.cellSize));
        int neighborCount = 0;
        Vector2 close = {};
        Vector2 avgVelocity = {};
        Vector2 avgPosition = {};
        for (auto [otherEntity, otherPosition, otherVelocity] : spatialHash.at(h)) {
            if (entity == otherEntity) continue;

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


void avoidance(entt::registry &reg, Config &config, const std::unordered_map<int, std::vector<HashEntry>> &spatialHash)
{
    const auto boids = reg.view<Boid, Position, Velocity>();

    for (auto [entity, position, velocity] : boids.each()) {
        Vector2 close = {};
        int h = hashCell(positionToCell(position, config.cellSize));
        for (auto [otherEntity, otherPosition, otherVelocity] : spatialHash.at(h)) {
            if (entity == otherEntity) continue;

            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.avoidRadius) {
                close = Vector2Add(close, distance);
            }
        }

        velocity.v = Vector2Add(velocity.v, Vector2Multiply(close, Vector2{config.avoidFactor, config.avoidFactor}));
    }
}

void alignment(entt::registry &reg, Config &config, const std::unordered_map<int, std::vector<HashEntry>> &spatialHash)
{
    reg.clear<Neighbor>();

    const auto boids = reg.view<Boid, Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        int h = hashCell(positionToCell(position, config.cellSize));
        Vector2 avgVelocity = {0};
        int neighborCount = 0;
        for (auto [otherEntity, otherPosition, otherVelocity] : spatialHash.at(h)) {
            if (entity == otherEntity) continue;

            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.visibleRadius) {
                neighborCount++;
                avgVelocity = Vector2Add(avgVelocity, otherVelocity.v);

                if (reg.all_of<Selected>(entity)) {
                    reg.emplace<Neighbor>(otherEntity);
                }
            }
        }

        if (neighborCount > 0) {
            avgVelocity = Vector2Divide(avgVelocity, Vector2{float(neighborCount), float(neighborCount)});
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Subtract(avgVelocity, velocity.v), Vector2{config.alignFactor, config.alignFactor}));
        }
    }
}

void cohesion(entt::registry &reg, Config &config, const std::unordered_map<int, std::vector<HashEntry>> &spatialHash)
{
    const auto boids = reg.view<Boid, Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        int h = hashCell(positionToCell(position, config.cellSize));
        Vector2 avgPosition = {0};
        int neighborCount = 0;
        for (auto [otherEntity, otherPosition, otherVelocity] : spatialHash.at(h)) {
            if (entity == otherEntity) continue;

            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.visibleRadius) {
                neighborCount++;
                avgPosition = Vector2Add(avgPosition, otherPosition.p);
            }
        }

        if (neighborCount > 0) {
            avgPosition = Vector2Divide(avgPosition, Vector2{float(neighborCount), float(neighborCount)});
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Subtract(avgPosition, position.p), Vector2{config.cohesionFactor, config.cohesionFactor}));
        }
    }
}

void mustGoFaster(entt::registry &reg, Config &config, float delta)
{
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

void drawBoids(entt::registry &reg)
{
    const auto boids = reg.view<Boid, Position, BoidColor>();

    for (auto [entity, position, color] : boids.each()) {
        auto c = color.color;
        if (reg.all_of<Candidate>(entity)) {
            c = GREEN;
        }
        if (reg.all_of<Neighbor>(entity)) {
            c = RED;
        }
        if (reg.all_of<Selected>(entity)) {
            c = BLUE;
        }

        auto size = Vector2{10, 10};
        auto halfSize = Vector2{size.x / 2.0f, size.y / 2.0f};
        DrawRectangleV(Vector2Subtract(position.p, halfSize), Vector2{10, 10}, c);
    }
}

int getSpatialRadius(const Config &config)
{
    int radius = 1;

    float maxRadiustoCheck = fmax(config.avoidRadius, config.visibleRadius);

    if (maxRadiustoCheck > config.cellSize) {
        radius = int(ceilf(maxRadiustoCheck / config.cellSize));
    }

    return radius + 1;
}

void markCandidates(entt::registry &reg, const Config &config, const std::unordered_map<int, std::vector<HashEntry>> &spatialHash)
{
    reg.clear<Candidate>();

    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        int h = hashCell(positionToCell(position, config.cellSize));
        for (auto entry : spatialHash.at(h)) {
            reg.emplace<Candidate>(entry.entity);
        }
    }
}

std::unordered_map<int, std::vector<HashEntry>> buildSpatialHash(entt::registry &reg, Config &config)
{
    std::unordered_map<int, std::vector<HashEntry>> results;

    const auto boids = reg.view<Boid, Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        auto cell = positionToCell(position, config.cellSize);

        int radius = getSpatialRadius(config);

        HashEntry entry = {entity, position, velocity};
        for (int y = cell.second - radius; y <= cell.second + radius; y++) {
            for (int x = cell.first - radius; x <= cell.first + radius; x ++) {
                results[hashCell(std::pair(x,y))].push_back(entry);
            }
        }
    }

    return results;
}

void drawSpatialHashGrid(entt::registry &reg, const Config &config)
{
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

void drawDebugLines(entt::registry &reg, const Config &config)
{
    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        DrawCircleLines(position.p.x, position.p.y, config.avoidRadius, RED);
        DrawCircleLines(position.p.x, position.p.y, config.visibleRadius, YELLOW);
    }
}

void selectBoid(entt::registry &reg, const Config &config, const std::unordered_map<int, std::vector<HashEntry>> &spatialHash)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();

        auto cell = positionToCell({mouse}, config.cellSize);
        int h = hashCell(cell);

        if (spatialHash.find(h) == spatialHash.end()) { return; }

        reg.clear<Selected>();

        entt::entity minEntity;
        float minDistance = FLT_MAX;
        for (auto [entity, position, veloocity] : spatialHash.at(hashCell(cell))) {
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
    DrawRectangleLines(config.bounds.x, config.bounds.y, config.bounds.width, config.bounds.height, RED);
}

void updateBounds(Config &config)
{
    int padding = 100;
    config.bounds.x = padding;
    config.bounds.y = padding;
    config.bounds.width = GetScreenWidth() - padding * 2;
    config.bounds.height = GetScreenHeight() - padding * 2;
}

int UpdateAndRender(GameData & data)
{
    float delta = GetFrameTime();

    updateBounds(data.config);

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        data.config.count++;
    }

    spawnBoids(data.reg, data.config);

    std::unordered_map<int, std::vector<HashEntry>> spatialHash = buildSpatialHash(data.reg, data.config);
    selectBoid(data.reg, data.config, spatialHash);
    markCandidates(data.reg, data.config, spatialHash);
    avoidance(data.reg, data.config, spatialHash);
    alignment(data.reg, data.config, spatialHash);
    cohesion(data.reg, data.config, spatialHash);
    // boidLogic(data.reg, data.config, spatialHash);
    updateTurnFactor(data.reg, data.config);
    mustGoFaster(data.reg, data.config, delta);
    moveEntities(data.reg, delta);

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(data.camera);
        drawBoids(data.reg);
        drawSpatialHashGrid(data.reg, data.config);
        drawDebugLines(data.reg, data.config);
        drawBounds(data.config);

        char buf[80];
        sprintf_s(buf, "boid count: %d", data.config.count);
        DrawText(buf, 10, 10, 20, Color{0, 255, 255, 255});

        sprintf_s(buf, "fps: %d", GetFPS());
        DrawText(buf, 10, 30, 20, Color{0, 255, 255, 255});

        EndMode2D();
    EndDrawing();
    //----------------------------------------------------------------------------------

    return 0;
}