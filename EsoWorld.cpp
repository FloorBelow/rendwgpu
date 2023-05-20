#include "EsoWorld.h"
#include <iostream>

char* Eso::World::WorldTocFilename(unsigned int world) {
    char* buffer = new char[20];
    sprintf(buffer, "%016llX.dat", 0x4400000000000000ULL | world);
    return buffer;
}

char* Eso::World::WorldCellFilename(unsigned int world, unsigned int layer, unsigned int x, unsigned int y) {
    char* buffer = new char[20];
    sprintf(buffer, "%016llX.dat", 0x4000000000000000ULL | ((world & 0x7FFULL) << 37) | ((layer & 0x1FULL) << 32) | ((x & 0xFFFFULL) << 16) | (y & 0xFFFFULL));
    return buffer;
}

Eso::Toc::Toc(char* path) {
    BinaryReader reader(path);
    //stream.seekg(4, std::ios_base::cur);
    std::cout << "POS " << reader.Pos() << "\n";
    reader.Seek(4);
    reader >> sizeX >> sizeY;



    //stream.seekg(4, std::ios_base::cur);


    /*




    std::cout << stream.peek() << " " << stream.tellg() << "\n";
    std::cout << stream.peek() << " " << stream.tellg() << "\n";
    */
}


Eso::FixtureFile::FixtureFile(char* path) {
    BinaryReader reader(path);
    reader >> version >> fixtureCount;
    //std::cout << "Fixture Cell Version " << version << "\n";
    std::cout << fixtureCount << " Fixtures\n";

    fixtures = new Fixture[fixtureCount];
    for (unsigned int i = 0; i < fixtureCount; i++) {
        reader >> fixtures[i].id;
        reader.Seek(8);
        reader >> fixtures[i].rotX >> fixtures[i].rotY >> fixtures[i].rotZ >> 
            fixtures[i].x >> fixtures[i].y >> fixtures[i].z;
        reader.Seek(12); //fixture header end
        reader.Seek(16); //static start

        reader >> fixtures[i].model;
        reader.Seek(8);
        if (version != 22) reader.Seek(16);
    }
}

Eso::TerrainLayer::TerrainLayer() {
    type = 0;
    rowSize = 0;
    rowCount = 0;
    data = nullptr;
}

void Eso::TerrainLayer::Read(BinaryReader& r, unsigned int type) {
    this->type = type;
    r.Seek(4);
    r >> rowCount;
    r.Seek(4);
    r >> rowSize;
    data = new char[rowCount * rowSize];
    for (unsigned int i = 0; i < rowCount; i++) {
        r.Seek(2);
        r.Read(data + i * rowSize, rowSize);

        //
    }
    r.Seek(4);
}

Eso::TerrainLayer::~TerrainLayer() {
    if(rowSize > 0)
    delete[] data;
}

Eso::TerrainFile::TerrainFile(const char* path) {
    BinaryReader reader(path);
    reader >> version;
    reader.Seek(7);
    reader >> layerCount;
    layerSizes = new unsigned int[layerCount];
    for (int i = 0; i < layerCount; i++) {
        reader.Seek(5);
        reader >> layerSizes[i];
    }
    reader.Seek(82);
    layers = new TerrainLayer[layerCount];
    for (int i = 0; i < layerCount; i++) {
        if (layerSizes[i] != 0) {
            //std::cout << "Reading layer " << i << "\n";
            layers[i].Read(reader, i);
        }
    }
}

Eso::TerrainFile::~TerrainFile() {
    delete layerSizes;
    delete[] layers;
}
