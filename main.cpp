#include "Renderer.hpp"
#include <vector>
#include <iostream>
#include <stdio.h>
#define WIDTH   20 //LCD width
#define HEIGHT  15 //LCD height

using std::cout;
using std::endl;

uint16_t FRAMEBUFFER[WIDTH * HEIGHT];

void showFrameBuffer()
{
    cout << "=========FRAMEBUFFER SIMULATOR==========" << endl;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%04x ", FRAMEBUFFER[j + i * WIDTH]);
        }
        cout << endl;
    }
}

int main()
{
    // Create a sample triangle
    auto v1 = Vertex(0, 5, 0);
    auto v2 = Vertex(5, 0, 0);
    auto v3 = Vertex(10, 5, 0);
    std::vector<Vertex> vertices;
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    auto tri = Triangle(vertices);
    Renderer renderer(FRAMEBUFFER, WIDTH, HEIGHT);
    renderer.drawTriangle(tri);
    showFrameBuffer();
    return 0;
}