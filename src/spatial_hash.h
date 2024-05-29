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

struct SpatialHash {
    typedef std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> underlying_set;
    std::unordered_map<cell, underlying_set, CellHash, CellEqual> hash;
    const Config *config;

    void insert(entt::entity e, Position p, Velocity v, LastPosition l, bool force = false);
    void remove(entt::entity e);
    const underlying_set &get_all_near_position(const Position &position) const;

    SpatialHash(const Config* config) : config(config) {};
};


void drawSpatialHashGrid(const entt::registry &reg, const SpatialHash &hash);