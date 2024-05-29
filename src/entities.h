#pragma once

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