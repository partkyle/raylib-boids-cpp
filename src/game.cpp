#include "game.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

int Init(GameData *data) {
    data->camera.zoom = 1.0;

    data->config.bounds = {0, 0, 1280, 720};

    data->config.count = 100;

    data->config.maxSpeed = 100.0f;
    data->config.turnFactor = 10;

    data->config.avoidRadius = 20.0f;
    data->config.avoidFactor = 0.05f;

    data->config.visibleRadius = 100.0f;
    data->config.alignFactor = 0.05f;
    data->config.cohesionFactor = 0.0005f;

    return 0;
}

struct Boid {};

struct Position {
    Vector2 p;
};

struct Velocity {
    Vector2 v;
};

float randf() {
    return rand() / float(RAND_MAX);
}

float lerp(float lo, float hi, float amount) {
    return amount * (hi - lo) + lo;
}

float randf_range(float lo, float hi) {
    return lerp(lo, hi, randf());
}

void spawnBoids(entt::registry &registry, const Config &config) {
    auto boids = registry.view<const Boid>();

    if (boids.size() < config.count) {
        for (int i = 0; i , config.count - boids.size(); i++) {
            const auto entity = registry.create();
            registry.emplace<Boid>(entity);
            registry.emplace<Position>(entity, Vector2{randf_range(0, config.bounds.width - config.bounds.x), randf_range(0, config.bounds.height - config.bounds.y)});
            registry.emplace<Velocity>(entity, Vector2{randf_range(-config.maxSpeed, config.maxSpeed), randf_range(-config.maxSpeed, config.maxSpeed)});
        }
    }
}

void updateTurnFactor(entt::registry &registry, Config &config) {
    const auto boids = registry.view<Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        if (position.p.x < config.bounds.x) {
            velocity.v.x += config.turnFactor;
        } else if (position.p.x > config.bounds.x + config.bounds.width) {
            velocity.v.x -= config.turnFactor;
        }

        if (position.p.y < config.bounds.y) {
            velocity.v.y += config.turnFactor;
        } else if (position.p.y > config.bounds.y + config.bounds.height) {
            velocity.v.y -= config.turnFactor;
        }
    }
}

void updateVelocity(entt::registry &registry, float deltaTime) {
    const auto boids = registry.view<Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        position.p = Vector2Add(position.p, Vector2Multiply(velocity.v, Vector2{deltaTime, deltaTime}));
    }
}

void avoidance(entt::registry &registry, Config &config) {
    const auto boids = registry.view<Boid, Position, Velocity>();

    Vector2 close = {0};
    for (auto [entity, position, velocity] : boids.each()) {
        for (auto [otherEntity, otherPosition, otherVelocity] : boids.each()) {
            if (entity == otherEntity) continue;
            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.avoidRadius) {
                close = Vector2Add(close, distance);
            }
        }

        velocity.v = Vector2Add(velocity.v, Vector2Multiply(close, Vector2{config.avoidFactor, config.avoidFactor}));
    }
}

void alignment(entt::registry &registry, Config &config) {
    const auto boids = registry.view<Boid, Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        Vector2 avgVelocity = {0};
        int neighborCount = 0;
        for (auto [otherEntity, otherPosition, otherVelocity] : boids.each()) {
            if (entity == otherEntity) continue;
            Vector2 distance = Vector2Subtract(position.p, otherPosition.p);
            if (Vector2Length(distance) <= config.visibleRadius) {
                neighborCount++;
                avgVelocity = Vector2Add(avgVelocity, otherVelocity.v);
            }
        }

        if (neighborCount > 0) {
            avgVelocity = Vector2Divide(avgVelocity, Vector2{float(neighborCount), float(neighborCount)});
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Subtract(avgVelocity, velocity.v), Vector2{config.alignFactor, config.alignFactor}));
        }
    }
}

void cohesion(entt::registry &registry, Config &config) {
    const auto boids = registry.view<Boid, Position, Velocity>();
    for (auto [entity, position, velocity] : boids.each()) {
        Vector2 avgPosition = {0};
        int neighborCount = 0;
        for (auto [otherEntity, otherPosition, otherVelocity] : boids.each()) {
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

void mustGoFaster(entt::registry &registry, Config &config, float delta) {
    const auto boids = registry.view<Boid, Velocity>();
    for (auto [entity, velocity] : boids.each()) {
        if (Vector2Length(velocity.v) < config.maxSpeed) {
            velocity.v = Vector2Add(velocity.v, Vector2Multiply(Vector2Multiply(Vector2Normalize(velocity.v), Vector2{config.maxSpeed, config.maxSpeed}), Vector2{delta, delta}));
        }
    }
}

void drawBoids(entt::registry &registry) {
    const auto boids = registry.view<Boid, Position>();

    for (auto [entity, position] : boids.each()) {
        DrawRectangleV(position.p, Vector2{10, 10}, GREEN);
    }
}

int UpdateAndRender(GameData * data)
{
    float delta = GetFrameTime();

    spawnBoids(data->registry, data->config);
    avoidance(data->registry, data->config);
    alignment(data->registry, data->config);
    cohesion(data->registry, data->config);
    updateTurnFactor(data->registry, data->config);
    mustGoFaster(data->registry, data->config, delta);
    updateVelocity(data->registry, delta);

    // Draw
    //----------------------------------------------------------------------------------
    BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(data->camera);
        drawBoids(data->registry);
        EndMode2D();
    EndDrawing();
    //----------------------------------------------------------------------------------

    return 0;
}