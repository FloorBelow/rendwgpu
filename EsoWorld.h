#pragma once
#include "BinaryReader.h"

namespace Eso {
    class World
    {
    public:
        static char* WorldTocFilename(unsigned int world);
        static char* WorldCellFilename(unsigned int world, unsigned int layer, unsigned int x, unsigned int y);
    };

    struct Toc {
        unsigned int sizeX;
        unsigned int sizeY;

        Toc(char* path);
    };

    struct Fixture {
        unsigned long long id;
        float x; float y; float z;
        float rotX; float rotY; float rotZ;
        unsigned int model;
    };

    struct FixtureFile {
        unsigned int version;
        unsigned int fixtureCount;
        Fixture* fixtures;

        FixtureFile(char* path);
    };

    struct TerrainLayer {
        unsigned int type;
        unsigned int rowSize;
        unsigned int rowCount;
        char* data;

        TerrainLayer();
        void Read(BinaryReader& r, unsigned int type);
        ~TerrainLayer();
    };

    struct TerrainFile {
        unsigned short version;
        unsigned char layerCount;
        unsigned int* layerSizes;
        TerrainLayer* layers;

        TerrainFile(const char* path);
        ~TerrainFile();
    };


}
