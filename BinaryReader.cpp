#include "BinaryReader.h"


BinaryReader::BinaryReader(const char* path) {
    stream = new std::ifstream(path, std::ios_base::binary);
}

BinaryReader::~BinaryReader() {
    delete stream;
}

void BinaryReader::Seek(int offset){
    stream->seekg(offset, std::ios_base::cur);
}

void BinaryReader::Read(char* buffer, int size) {
    stream->read(buffer, size);
}

int BinaryReader::Pos()
{
    return (int)stream->tellg();
}

BinaryReader& operator>>(BinaryReader& reader, unsigned char& c) {
    reader.stream->read((char*)&c, 1);
    return reader;
}

BinaryReader& operator>>(BinaryReader& reader, unsigned short& s) {
    reader.stream->read((char*)&s, 2);
    return reader;
}
BinaryReader& operator >>(BinaryReader& reader, unsigned int& i) {
    reader.stream->read((char*)&i, 4);
    return reader;
}
BinaryReader& operator >>(BinaryReader& reader, unsigned long long& l) {
    reader.stream->read((char*)&l, 8);
    return reader;
}
BinaryReader& operator >>(BinaryReader& reader, float& f) {
    reader.stream->read((char*)&f, 4);
    return reader;
}