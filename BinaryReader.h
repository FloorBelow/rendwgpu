#include <fstream>

class BinaryReader {
public:
    std::ifstream* stream;

    BinaryReader(const char* path);
    ~BinaryReader();


    void Seek(int offset);
    void Read(char* buffer, int size);
    int Pos();
};

BinaryReader& operator >> (BinaryReader& reader, unsigned char& c);
BinaryReader& operator >> (BinaryReader& reader, unsigned short& s);
BinaryReader& operator >> (BinaryReader& reader, unsigned int& i);
BinaryReader& operator >> (BinaryReader& reader, unsigned long long& l);
BinaryReader& operator >> (BinaryReader& reader, float& f);
