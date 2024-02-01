#ifndef TRIANGLE_DRAW_H
#define TRIANGLE_DRAW_H

#include <vector>

class Vertex
{
  public:
    inline Vertex(int x, int y, int z) : x_ { x }, y_ { y }, z_ { z } {};
    inline int x() const { return x_; }
    inline int y() const { return y_; }
    inline int z() const { return z_; }
    // Return true if the y axis of this object is gte the compared object
    bool operator<(const Vertex &v)
    {
        return y_ < v.y();
    }

  private:
    int x_;
    int y_;
    int z_;
};

enum TriangleType {
    FLAT_TOP,
    FLAT_BOTTOM,
    IRREGULAR
};

class Triangle
{
  public:
    Triangle(std::vector<Vertex> vertices);
    Vertex &upperVertex() { return upperVertex_; }
    Vertex &middleVertex() { return middleVertex_; }
    Vertex &lowerVertex() { return lowerVertex_; }
    TriangleType type() { return type_; }
  private:
    Vertex &upperVertex_;
    Vertex &middleVertex_;
    Vertex &lowerVertex_;
    TriangleType type_;
};

#endif