#include "triangleDraw.h"
#include <algorithm>

Triangle::Triangle(std::vector<Vertex> vertices) {
    // Add an assert to verify only 3 vertices
    std::sort(vertices.begin(), vertices.end());
    // Assign vertices from sorted vector
    upperVertex_ = vertices[0];
    middleVertex_ = vertices[1];
    lowerVertex_ = vertices[2];

    // Determine triangle type
    if (upperVertex_.y() == middleVertex_()) type_ = TriangleType::FLAT_BOTOM;
    else if (lowerVertex_.y() == middleVertex_()) type_ = TriangleType::FLAT_TOP;
    else type_ = TriangleType::IRREGULAR;
}
