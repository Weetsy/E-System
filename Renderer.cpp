#include "Renderer.hpp"

Renderer::Renderer(uint16_t *frameBuffer, unsigned int width, unsigned int height) : width_ { width },
    height_ { height }, frameBuffer_ { frameBuffer } {

}

RenderResult Renderer::drawLine(unsigned int x1, unsigned int x2, unsigned int y)
{
    // Draw the given line onto the frame buffer
    if (x1 > width_) return RenderResult::FAILURE;
    if (x2 > width_) return RenderResult::FAILURE;
    if (y > height_) return RenderResult::FAILURE;

    // Write color data into the frame buffer
    for (unsigned int i = x1; i < x2; i++)
    {
        frameBuffer_[y * width_ + i] = 0xFFFF;
    }
}

RenderResult Renderer::renderFlatTopTriangle(float is1, float is2, float d1, float d2, int y)
{
    if ((int)d1 == (int)d2) {
        // Render the final pixel at the apex of the triangle, and then return
        drawLine((int)d1, (int)d2, y);
        return RenderResult::OK;
    }
    // Render the triangle to the top vertex
    // Draw the line between d1 and d2
    drawLine((int)d1, (int)d2, y);
    // Shift upwards
    unsigned int y2 = y - 1;
    // Calculate the shift between domains on X
    float nd1 = d1 - is1;
    float nd2 = d2 - is2;

    renderFlatTopTriangle(is1, is2, nd1, nd2, y2);
}

/// @todo Don't use floats here...
float calcluateSlope(Vertex &v1, Vertex &v2)
{
    float dx = v2.x() - v1.x();
    float dy = v2.y() - v2.y();

    return dx / dy; 
}

RenderResult Renderer::drawTriangle(Triangle &triangle)
{
    float s1, s2;
    float d1, d2;
    int y;
    // Switch rendering mode based on triangle type
    switch (triangle.type())
    {
        case TriangleType::FLAT_TOP:
            // Calculate the inverse slope from mid to high
            s1 = calcluateSlope(triangle.middleVertex(), triangle.upperVertex());
            // Calculate the inverse slope from low to high
            s2 = calcluateSlope(triangle.lowerVertex(), triangle.upperVertex());

            // Set the domain with respect to the slopes above
            d1 = triangle.middleVertex().x();
            d2 = triangle.lowerVertex().x();
            y = triangle.lowerVertex().y(); // Lower and middle should be the same...

            renderFlatTopTriangle(s1, s2, d1, d2, y);
            break;
        case TriangleType::FLAT_BOTTOM:
        case TriangleType::IRREGULAR:
            break;
        default:
            break;
    }
    return RenderResult::OK;
}
