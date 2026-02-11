/* Release code for program 1 CPE 471 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>

#include "tiny_obj_loader.h"
#include "Image.h"
#include "Triangle.h"

// This allows you to skip the `std::` in front of C++ standard library
// functions. You can also say `using std::cout` to be more selective.
// You should never do this in a header file.
using namespace std;

int g_width, g_height;

/*
   Helper function you will want all quarter
   Given a vector of shapes which has already been read from an obj file
   resize all vertices to the range [-1, 1]
 */
void resize_obj(std::vector<tinyobj::shape_t> &shapes)
{
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
    float scaleX, scaleY, scaleZ;
    float shiftX, shiftY, shiftZ;
    float epsilon = 0.001;

    minX = minY = minZ = 1.1754E+38F;
    maxX = maxY = maxZ = -1.1754E+38F;

    // Go through all vertices to determine min and max of each dimension
    for (size_t i = 0; i < shapes.size(); i++)
    {
        for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++)
        {
            if (shapes[i].mesh.positions[3 * v + 0] < minX)
                minX = shapes[i].mesh.positions[3 * v + 0];
            if (shapes[i].mesh.positions[3 * v + 0] > maxX)
                maxX = shapes[i].mesh.positions[3 * v + 0];

            if (shapes[i].mesh.positions[3 * v + 1] < minY)
                minY = shapes[i].mesh.positions[3 * v + 1];
            if (shapes[i].mesh.positions[3 * v + 1] > maxY)
                maxY = shapes[i].mesh.positions[3 * v + 1];

            if (shapes[i].mesh.positions[3 * v + 2] < minZ)
                minZ = shapes[i].mesh.positions[3 * v + 2];
            if (shapes[i].mesh.positions[3 * v + 2] > maxZ)
                maxZ = shapes[i].mesh.positions[3 * v + 2];
        }
    }

    // From min and max compute necessary scale and shift for each dimension
    float maxExtent, xExtent, yExtent, zExtent;
    xExtent = maxX - minX;
    yExtent = maxY - minY;
    zExtent = maxZ - minZ;
    if (xExtent >= yExtent && xExtent >= zExtent)
    {
        maxExtent = xExtent;
    }
    if (yExtent >= xExtent && yExtent >= zExtent)
    {
        maxExtent = yExtent;
    }
    if (zExtent >= xExtent && zExtent >= yExtent)
    {
        maxExtent = zExtent;
    }
    scaleX = 2.0 / maxExtent;
    shiftX = minX + (xExtent / 2.0);
    scaleY = 2.0 / maxExtent;
    shiftY = minY + (yExtent / 2.0);
    scaleZ = 2.0 / maxExtent;
    shiftZ = minZ + (zExtent) / 2.0;

    // Go through all verticies shift and scale them
    for (size_t i = 0; i < shapes.size(); i++)
    {
        for (size_t v = 0; v < shapes[i].mesh.positions.size() / 3; v++)
        {
            shapes[i].mesh.positions[3 * v + 0] = (shapes[i].mesh.positions[3 * v + 0] - shiftX) * scaleX;
            assert(shapes[i].mesh.positions[3 * v + 0] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 0] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3 * v + 1] = (shapes[i].mesh.positions[3 * v + 1] - shiftY) * scaleY;
            assert(shapes[i].mesh.positions[3 * v + 1] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 1] <= 1.0 + epsilon);
            shapes[i].mesh.positions[3 * v + 2] = (shapes[i].mesh.positions[3 * v + 2] - shiftZ) * scaleZ;
            assert(shapes[i].mesh.positions[3 * v + 2] >= -1.0 - epsilon);
            assert(shapes[i].mesh.positions[3 * v + 2] <= 1.0 + epsilon);
        }
    }
}

/*
    Bresenham's line drawing algorithm with z-buffering
*/
void drawLine(Vec3 v0, Vec3 v1, shared_ptr<Image> outImage, vector<float> &zBuffer)
{
    int x0 = static_cast<int>(v0.getX());
    int y0 = static_cast<int>(v0.getY());
    int x1 = static_cast<int>(v1.getX());
    int y1 = static_cast<int>(v1.getY());
    float z0 = v0.getZ();
    float z1 = v1.getZ();

    // Calculate differences and steps
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    // Determine the direction of the increment
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    // Initialize length and step for interpolation
    int length = max(dx, dy);
    int step = 0;
    float zMin = -1.0f, zMax = 1.0f;
    while (true)
    {
        // Interpolate z
        float t = (length == 0) ? 0.0f : static_cast<float>(step) / length;
        float z = z0 * (1.0f - t) + z1 * t;

        int idx = y0 * g_width + x0;
        if (z > zBuffer[idx])
        {
            zBuffer[idx] = z;
            // map z to desired rgb values
            unsigned char red = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 100.0f);
            unsigned char green = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 200.0f);
            unsigned char blue = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 200.0f);
            outImage->setPixel(x0, y0, red, green, blue);
        }

        // Check if reached the end point
        if (x0 == x1 && y0 == y1)
            break;

        // Calculate error term
        int err2 = 2 * err;
        if (err2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (err2 < dx)
        {
            err += dx;
            y0 += sy;
        }
        step++;
    }
}

/*
    write out image (PNG) data, using the triangles bounding box
    Depending on mode, either fill the triangle using z-buffering (mode 0)
    or draw the triangle edges using Bresenham's line algorithm (mode 1)
*/
void draw(shared_ptr<Image> outImage, Triangle triangle, vector<float> &zBuffer, int mode)
{
    triangle.computeBBox();

    // for every point in the bounding Box of the triangle
    if (mode == 0)
    {
        float xa = triangle.getX0();
        float ya = triangle.getY0();
        float xb = triangle.getX1();
        float yb = triangle.getY1();
        float xc = triangle.getX2();
        float yc = triangle.getY2();
        float zMin = -1.0f, zMax = 1.0f;
        for (int y = triangle.getBBox().minY; y <= triangle.getBBox().maxY; y++)
        {
            for (int x = triangle.getBBox().minX; x <= triangle.getBBox().maxX; x++)
            {
                float beta = ((xa - xc) * (y - yc) - (x - xc) * (ya - yc)) /
                             ((xb - xa) * (yc - ya) - (xc - xa) * (yb - ya));
                float gamma = ((xb - xa) * (y - ya) - (x - xa) * (yb - ya)) /
                              ((xb - xa) * (yc - ya) - (xc - xa) * (yb - ya));
                float alpha = 1.0f - beta - gamma;
                if (0 <= alpha && alpha <= 1 && 0 <= beta && beta <= 1 && 0 <= gamma && gamma <= 1)
                {
                    // update rasterization if the z value is larger
                    float z = alpha * triangle.getZ0() + beta * triangle.getZ1() + gamma * triangle.getZ2();
                    int idx = y * g_width + x;
                    if (z > zBuffer[idx])
                    {
                        zBuffer[idx] = z;
                        // map z to desired rgb values
                        unsigned char red = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 100.0f);
                        unsigned char green = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 200.0f);
                        unsigned char blue = static_cast<unsigned char>((z - zMin) / (zMax - zMin) * 200.0f);
                        outImage->setPixel(x, y, red, green, blue);
                    }
                }
            }
        }
    }
    else
    {
        drawLine(triangle.getV0(), triangle.getV1(), outImage, zBuffer);
        drawLine(triangle.getV1(), triangle.getV2(), outImage, zBuffer);
        drawLine(triangle.getV2(), triangle.getV0(), outImage, zBuffer);
    }
}

int main(int argc, char **argv)
{
    if (argc != 6)
    {
        cout << "Usage: " << argv[0] << " <meshfile> <imagefile> <width> <height> <mode>" << endl;
        return 0;
    }

    // OBJ filename
    string meshName(argv[1]);
    string imgName(argv[2]);

    // set g_width and g_height appropriately!
    g_width = stoi(argv[3]);
    g_height = stoi(argv[4]);
    int mode = stoi(argv[5]);

    if (mode != 0 && mode != 1)
    {
        cout << "Invalid mode: " << mode << endl;
        return 0;
    }

    // create an image
    auto image = make_shared<Image>(g_width, g_height);

    // triangle buffer
    vector<unsigned int> triBuf;
    // position buffer
    vector<float> posBuf;
    // Some obj files contain material information.
    // We'll ignore them for this assignment.
    vector<tinyobj::shape_t> shapes;          // geometry
    vector<tinyobj::material_t> objMaterials; // material
    string errStr;

    bool rc = tinyobj::LoadObj(shapes, objMaterials, errStr, meshName.c_str());
    /* error checking on read */
    if (!rc)
    {
        cerr << errStr << endl;
    }
    else
    {
        // keep this code to resize your object to be within -1 -> 1
        resize_obj(shapes);
        posBuf = shapes[0].mesh.positions;
        triBuf = shapes[0].mesh.indices;
    }
    cout << "Number of vertices: " << posBuf.size() / 3 << endl;
    cout << "Number of triangles: " << triBuf.size() / 3 << endl;

    // iterate through each triangle and rasterize it
    float translationX = (g_width - 1) * 0.5f;
    float translationY = (g_height - 1) * 0.5f;
    float scale = (min(g_width, g_height) - 1) * 0.5f;
    std::vector<float> zBuffer(g_width * g_height, -std::numeric_limits<float>::infinity());
    for (size_t i = 0; i < triBuf.size(); i += 3)
    {
        Triangle triangle = Triangle(posBuf, triBuf, i, translationX, translationY, scale);
        draw(image, triangle, zBuffer, mode);
    }

    // write out the image
    image->writeToFile(imgName);

    return 0;
}
