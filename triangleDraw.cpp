#include "triangleDraw.h"
#include <algorithm>

Triangle::Triangle(std::vector<Vertex> &vertices) : vertices_ { vertices } {
    // Add an assert to verify only 3 vertices
    std::sort(vertices.begin(), vertices.end());

    // Determine triangle type
    if (upperVertex().y() == middleVertex().y()) type_ = TriangleType::FLAT_BOTTOM;
    else if (lowerVertex().y() == middleVertex().y()) type_ = TriangleType::FLAT_TOP;
    else type_ = TriangleType::IRREGULAR;
}
