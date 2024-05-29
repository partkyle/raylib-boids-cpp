#include "tracy/Tracy.hpp"

#include "spatial_hash.h"

std::pair<int, int> positionToCell(const Position &p, float cellSize)
{
    return std::pair(int(p.p.x / cellSize),  int(p.p.y / cellSize));
}

int hashCell(std::pair<int, int> p) {
    return p.first * 92837111 + p.second * 689287499;
}

int getSpatialRadius(const Config *config)
{
    int radius = 1;

    float maxRadiustoCheck = fmax(config->avoidRadius, config->visibleRadius);

    if (maxRadiustoCheck > config->cellSize) {
        radius = int(ceilf(maxRadiustoCheck / config->cellSize));
    }

    return radius;
}

void drawSpatialHashGrid(const entt::registry& reg, const SpatialHash &hash)
{
    ZoneScoped;

    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        float cellSize = hash.config->cellSize;
        auto cell = positionToCell(position, cellSize);
        int radius = getSpatialRadius(hash.config);
        for (int y = cell.second - radius; y <= cell.second + radius; y++) {
            for (int x = cell.first - radius; x <= cell.first + radius; x++) {
                DrawRectangleLines(int(floorf(x * cellSize)), int(floorf(y * cellSize)), int(cellSize), int(cellSize), RED);
            }
        }
    }
}

void SpatialHash::insert(entt::entity e, Position p, Velocity v, LastPosition l, bool force)
{
    ZoneScoped;

    int radius = getSpatialRadius(config);

    auto newCellPos = positionToCell(p, config->cellSize);
    auto lastPos = positionToCell({ l.p }, config->cellSize);

    // if we aren't forcing it, we will remove it.
    if (!force && newCellPos.first == lastPos.first && newCellPos.second == lastPos.second) return;

    for (int y = lastPos.second - radius; y <= lastPos.second + radius; y++) {
        for (int x = lastPos.first - radius; x <= lastPos.first + radius; x++) {
            hash[cell(x, y)].erase(e);
        }
    }

    {
        for (int y = newCellPos.second - radius; y <= newCellPos.second + radius; y++) {
            for (int x = newCellPos.first - radius; x <= newCellPos.first + radius; x++) {
                hash[cell(x, y)].insert(e);
            }
        }
    }
}

void SpatialHash::remove(entt::entity e)
{
    ZoneScoped;

    for (auto& [_key, value] : hash) {
        value.erase(e);
    }
}

const SpatialHash::underlying_set emptySet;

const SpatialHash::underlying_set& SpatialHash::get_all_near_position(const Position& position) const
{
    ZoneScoped;

    auto cell = positionToCell(position, config->cellSize);

    if (hash.find(cell) == hash.end()) return emptySet;

    return hash.at(cell);
}
