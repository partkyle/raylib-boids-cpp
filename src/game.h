#pragma once

#include <entt/entt.hpp>

#include "raylib.h"

#include "tracy/Tracy.hpp"

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

struct Boid {};

struct Position
{
    Vector2 p;
};

struct LastPosition
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

struct HashEntry {
    entt::entity entity;
};

struct HashEntryHash {
    size_t operator()(const HashEntry &h) const
    {
        int result = 0;
        return std::_Fnv1a_append_value(result, h.entity);
    }
};

struct HashEntryEqual
{
    constexpr bool operator()(const HashEntry& l, const HashEntry& r) const
    {
        return l.entity == r.entity;
    }
};

static std::pair<int, int> positionToCell(const Position &p, float cellSize)
{
    return std::pair(int(p.p.x / cellSize),  int(p.p.y / cellSize));
}

static int hashCell(std::pair<int, int> p) {
    return p.first * 92837111 + p.second * 689287499;
}

typedef std::pair<int,int> cell;

struct CellHash
{
    size_t operator()(const cell &cell) const
    {
        int result = 0;
        result = std::_Fnv1a_append_value(result, cell.first);
        return std::_Fnv1a_append_value(result, cell.second);
    }
};

struct CellEqual
{
    constexpr bool operator()(const cell& l, const cell& r) const
    {
        return l.first == r.first && l.second == r.second;
    }
};

typedef std::unordered_map<cell, std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual>, CellHash, CellEqual> SpatialHash;

static int getSpatialRadius(const Config &config)
{
    int radius = 1;

    float maxRadiustoCheck = fmax(config.avoidRadius, config.visibleRadius);

    if (maxRadiustoCheck > config.cellSize) {
        radius = int(ceilf(maxRadiustoCheck / config.cellSize));
    }

    return radius;
}

static void insert_into(SpatialHash &hash, const Config &config, entt::entity e, Position p, Velocity v, LastPosition l, bool force = false)
{
    ZoneScoped;

    int radius = getSpatialRadius(config);

    auto newCellPos = positionToCell(p, config.cellSize);
    auto lastPos = positionToCell({l.p}, config.cellSize);

    // if we aren't forcing it, we will remove it.
    if (!force && newCellPos.first == lastPos.first && newCellPos.second == lastPos.second) return;

    auto entry = HashEntry{ e };
    
    for (int y = lastPos.second - radius; y <= lastPos.second + radius; y++) {
        for (int x = lastPos.first - radius; x <= lastPos.first + radius; x++) {
            hash[cell(x, y)].erase(entry);
        }
    }

    {
        for (int y = newCellPos.second - radius; y <= newCellPos.second + radius; y++) {
            for (int x = newCellPos.first - radius; x <= newCellPos.first + radius; x++) {
                hash[cell(x, y)].insert(entry);
            }
        }
    }
}

static void remove_from(SpatialHash& hash, const Config& config, entt::entity e)
{
    ZoneScoped;

    for (auto &[_key, value] : hash) {
        value.erase(HashEntry{ e });
    }
}


const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> emptySet;

static const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> &get_all_in_cell(const SpatialHash &hash, const Config &config, int cellx, int celly)
{  
    ZoneScoped;

    auto key = cell(cellx, celly);

    if (hash.find(key) == hash.end()) return emptySet;

    return hash.at(key);
}

struct GameData {
    entt::registry reg;

    Camera2D camera;

    Config config;

    SpatialHash spatialHash;
};


int Init(GameData &data);
int UpdateAndRender(GameData &data);