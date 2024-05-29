#include "tracy/Tracy.hpp"

#include "spatial_hash.h"

std::pair<int, int> positionToCell(const Position &p, float cellSize)
{
    return std::pair(int(p.p.x / cellSize),  int(p.p.y / cellSize));
}

int hashCell(std::pair<int, int> p) {
    return p.first * 92837111 + p.second * 689287499;
}

int getSpatialRadius(const Config &config)
{
    int radius = 1;

    float maxRadiustoCheck = fmax(config.avoidRadius, config.visibleRadius);

    if (maxRadiustoCheck > config.cellSize) {
        radius = int(ceilf(maxRadiustoCheck / config.cellSize));
    }

    return radius;
}

void insert_into(SpatialHash &hash, const Config &config, entt::entity e, Position p, Velocity v, LastPosition l, bool force)
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

void remove_from(SpatialHash& hash, const Config& config, entt::entity e)
{
    ZoneScoped;

    for (auto &[_key, value] : hash) {
        value.erase(HashEntry{ e });
    }
}

const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> emptySet;

const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual> &get_all_in_cell(const SpatialHash &hash, const Config &config, int cellx, int celly)
{  
    ZoneScoped;

    auto key = cell(cellx, celly);

    if (hash.find(key) == hash.end()) return emptySet;

    return hash.at(key);
}

const std::unordered_set<HashEntry, HashEntryHash, HashEntryEqual>& get_all_near_position(const SpatialHash& hash, const Config& config, const Position& position)
{
    // TODO: insert return statement here
    auto cell = positionToCell(position, config.cellSize);
    return get_all_in_cell(hash, config, cell.first, cell.second);
}

void drawSpatialHashGrid(const entt::registry& reg, const Config& config)
{
    ZoneScoped;

    auto selected = reg.view<Position, Selected>();
    for (auto [entity, position] : selected.each()) {
        auto cell = positionToCell(position, config.cellSize);
        int radius = getSpatialRadius(config);
        for (int y = cell.second - radius; y <= cell.second + radius; y++) {
            for (int x = cell.first - radius; x <= cell.first + radius; x++) {
                DrawRectangleLines(int(floorf(x * config.cellSize)), int(floorf(y * config.cellSize)), int(config.cellSize), int(config.cellSize), RED);
            }
        }
    }
}
