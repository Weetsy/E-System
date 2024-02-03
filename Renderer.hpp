#ifndef RENDERER_H
#define RENDERER_H

#include "triangleDraw.h"
typedef unsigned short uint16_t;

enum RenderResult
{
    FAILURE,
    OK
};

class Renderer
{
  public:
    Renderer(uint16_t *frameBuffer, unsigned int width, unsigned int height);
    RenderResult drawLine(unsigned int x1, unsigned int x2, unsigned int y);
    RenderResult drawTriangle(Triangle &triangle);
    RenderResult renderFlatTopTriangle(float is1, float is2, float d1, float d2, int y);
    RenderResult renderFlatBottomTriangle(float is1, float is2, float d1, float d2, int y);
  private:
    unsigned int width_;
    unsigned int height_;
    uint16_t *frameBuffer_;
};

#endif