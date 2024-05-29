#pragma once

#include <entt/entt.hpp>

#include "config.h"
#include "entities.h"

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

void insert_into(SpatialHash &hash, const Config &config, entt::entity e, Position p, Velocity v, LastPosition l, bool force = false);
void remove_from(SpatialHash &hash, const Config &config, entt::entity e);
const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> &get_all_near_position(const SpatialHash &hash, const Config &config, const Position &position);
void drawSpatialHashGrid(const entt::registry& reg, const Config& config);