#include "Renderer.hpp"
#include <iostream>
#include <stdio.h>
#include <vector>
#include <cmath>
#define VERBOSE
// Pray to god this is 32 bits
typedef unsigned int uint32_t;
using std::cout;
using std::endl;
Renderer::Renderer(uint16_t *frameBuffer, unsigned int width, unsigned int height) : width_ { width },
    height_ { height }, frameBuffer_ { frameBuffer }, color_ { 0xFFFF } {

}

RenderResult Renderer::drawLine(int x1, int x2, int y)
{
    //cout << "Renderer::drawLine entry" << endl;
    // Draw the given line onto the frame buffer
    if (x1 > width_ || x1 < 0) x1 = 0;
    if (x2 > width_ || x2 < 0) x2 = 0;
    if (y > height_ || y < 0) y = 0;

    // Write color data into the frame buffer
    for (uint32_t i = x1; i < x2; i++)
    {
        frameBuffer_[y * width_ + i] = color_;
    }
    return RenderResult::OK;
}

RenderResult Renderer::renderFlatTopTriangle(float is1, float is2, float d1, float d2, int y)
{
#ifdef VERBOSE
    printf("Renderer::renderFlatTopTriangle: is1 %f, is2 %f, d1 %f, d2 %f, y %d\n",
        is1,
        is2,
        d1,
        d2,
        y);
#endif
    if ((int)std::round(d1) == (int)std::round(d2)) {
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
    return RenderResult::OK;
}

RenderResult Renderer::renderFlatBottomTriangle(float is1, float is2, float d1, float d2, int y)
{
#ifdef VERBOSE
    printf("Renderer::renderFlatBottomTriangle: is1 %f, is2 %f, d1 %f, d2 %f, y %d\n",
        is1,
        is2,
        d1,
        d2,
        y);
#endif
    if ((int)d1 == (int)d2) {
        // Render the final pixel at the apex of the triangle, and then return
        drawLine((int)d1, (int)d2, y);
        return RenderResult::OK;
    }
    // Render the triangle to the top vertex
    // Draw the line between d1 and d2
    drawLine((int)d1, (int)d2, y);
    // Shift downwards
    unsigned int y2 = y + 1;
    // Calculate the shift between domains on X
    float nd1 = d1 + is1;
    float nd2 = d2 + is2;

    renderFlatBottomTriangle(is1, is2, nd1, nd2, y2);
    return RenderResult::OK;
}

/// @todo Don't use floats here...
float calcluateSlope(Vertex &v1, Vertex &v2)
{
    float dx = v2.x() - v1.x();
    float dy = v2.y() - v1.y();
#ifdef VERBOSE
    printf("calculateSlope: dx: %f, dy: %f\n",
        dx,
        dy);
#endif
    if (dy == 0.0f) return 0.0f;
    return dx / dy; 
}

RenderResult Renderer::drawTriangle(Triangle &triangle, uint16_t color)
{
    float s1, s2;
    float d1, d2;
    int y;
    color_ = color;
    // Switch rendering mode based on triangle type
    switch (triangle.type()) {
        case TriangleType::FLAT_TOP:
            cout << "Renderer::drawTriangle: Rendering FLAT_TOP Triangle" << endl;
            // Calculate the inverse slope from mid to high
            s1 = calcluateSlope(triangle.middleVertex(), triangle.upperVertex());
            // Calculate the inverse slope from low to high
            s2 = calcluateSlope(triangle.lowerVertex(), triangle.upperVertex());

            // Set the domain with respect to the slopes above
            d1 = triangle.middleVertex().x();
            d2 = triangle.lowerVertex().x();
            y = triangle.lowerVertex().y();  // Lower and middle should be the same...

            if (d1 > d2) {
                renderFlatTopTriangle(s2, s1, d2, d1, y);
            } else {
                renderFlatTopTriangle(s1, s2, d1, d2, y);
            }
            break;
        case TriangleType::FLAT_BOTTOM:
            cout << "Renderer::drawTriangle: Rendering FLAT_BOTTOM Triangle" << endl;
            // Calculate the inverse slope from mid to high
            s1 = calcluateSlope(triangle.middleVertex(), triangle.lowerVertex());
            // Calculate the inverse slope from low to high
            s2 = calcluateSlope(triangle.upperVertex(), triangle.lowerVertex());

            // Set the domain with respect to the slopes above
            d1 = triangle.middleVertex().x();
            d2 = triangle.upperVertex().x();
            y = triangle.upperVertex().y();

            if (d1 > d2) {
                renderFlatBottomTriangle(s2, s1, d2, d1, y);
            } else {
                renderFlatBottomTriangle(s1, s2, d1, d2, y);
            }

            break;
        case TriangleType::IRREGULAR:
        {
            cout << "Renderer::drawTriangle: Rendering IRREGULAR Triangle" << endl;
            auto slope = static_cast<float>((triangle.upperVertex().y() - triangle.lowerVertex().y())) /
                static_cast<float>((triangle.upperVertex().x() - triangle.lowerVertex().x()));
            // Calculate B, then create function
            // y = mx + b
            // y = upperVertex.y
            // m = slope
            // x = upperVertex.x
            // b = y - mx
            auto b = triangle.upperVertex().y() - (slope * triangle.upperVertex().x());
            // Use B to calculate vertex 4 point (only care about X rn... do the same for Z later)
            // y = mx + b
            // middleVertex.y = (slope * x) + b
            // slope * x = middleVertex.y - b
            // x = (middleVertex.y - b) / slope
            auto x = (triangle.middleVertex().y() - b) / slope;
            /// @todo Z should be calculated for v4, but I'm lazy and will do that later
            auto v4 = Vertex(x, triangle.middleVertex().y(), triangle.middleVertex().z());
            // Generate the new vertex point to create both triangles
            std::vector<Vertex> flatBottomVertices = {
                v4,
                triangle.upperVertex(),
                triangle.middleVertex()
            };

            auto flatBottomTri = Triangle(flatBottomVertices);

            std::vector<Vertex> flatTopVertices = {
                v4,
                triangle.lowerVertex(),
                triangle.middleVertex()
            };

            auto flatTopTri = Triangle(flatTopVertices);
            // Recurse
            auto res = drawTriangle(flatTopTri, color);
            if (res != RenderResult::OK) {
                return res;
            }
            res = drawTriangle(flatBottomTri, color);
            if (res != RenderResult::OK) {
                return res;
            }
            break;
        }
        default:
            cout << "Renderer::drawTriangle: Unsupported render mode" << endl;
            break;
    }
    return RenderResult::OK;
}
