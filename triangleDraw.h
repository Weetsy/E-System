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
    Triangle(std::vector<Vertex> &vertices);
    Vertex &upperVertex() { return vertices_[0]; }
    Vertex &middleVertex() { return vertices_[1]; }
    Vertex &lowerVertex() { return vertices_[2]; }
    TriangleType type() { return type_; }
  private:
    std::vector<Vertex> &vertices_;
    TriangleType type_;
};

#endif