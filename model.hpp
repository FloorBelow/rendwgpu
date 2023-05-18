#include "webgpu\webgpu.hpp"

struct Model {
public:
    //idx buffers have to be a mult of 16 so this is neccecary, to tell in the render pass how much to use from each buffer
	int vertBufferSize;
	int idxBufferSize;
	int idxCount;
	bool idx32;

	wgpu::Buffer vertBuffer = nullptr;
	wgpu::Buffer idxBuffer = nullptr;

	std::vector<wgpu::VertexAttribute> vertAttributes;

	Model(const char* path, wgpu::Device& device, wgpu::Queue& queue);

private:
	char* vertData;
	char* idxData;


	struct EsoVert {
		float x; //4
		float y; //8
		float z; //12
		char r;  //13
		char g;  //14
		char b;  //15
		char a;  //16
		short nx;//18
		short ny;//20
		short tx;//22
		short ty;//24
		short bx;//26
		short by;//28
		short u; //30
		short v; //32
	};
};