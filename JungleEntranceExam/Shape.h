#pragma once

// 정점 구조체 선언
struct FVertexSimple
{
    float x, y, z;    // Position
    float r, g, b, a; // Color
};

// Structure for a 3D vector
struct FVector
{
    float x, y, z;
    FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
};

struct FColor
{
    float r, g, b, a;
    FColor(float _r = 0, float _g = 0, float _b = 0, float _a = 1.0f)
        : r(_r), g(_g), b(_b), a(_a) {
    }

    FColor operator+(float num) const 
    {
        return { r + num, g + num, b + num, a };
    }
    FColor operator+(const FColor& c) const
    {
        return { r + c.r, g + c.g, b + c.b, a };
    }
    FColor operator-(float num) const 
    {
        return { r - num, g - num, b - num, a };
    }
    FColor operator-(const FColor& c) const
    {
        return { r - c.r, g - c.g, b - c.b, a };
    }
    FColor operator*(float num) const
    {
        return { r * num, g * num, b * num, a };
    }
    FColor operator*(const FColor& c) const
    {
        return { r * c.r, g * c.g, b * c.b, a }; 
    }

 };