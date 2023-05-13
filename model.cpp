#include "model.hpp"
#include "granny2\include\granny.h"
#include "webgpu\webgpu.hpp"
using namespace wgpu;

Model::Model(const char* path, Device& device, Queue& queue) {
	//jank granny testing stuff
	granny_file* file = GrannyReadEntireFile(path);
	granny_file_info* info = GrannyGetFileInfo(file);
	std::cout << info->Meshes[0]->Name << std::endl;
	granny_mesh* grannyMesh = info->Meshes[0];


	//int grannyVertCount = grannyMesh->PrimaryVertexData->VertexCount;
	//std::vector<float> grannyVertData(grannyVertCount * 6);
	//GrannyCopyMeshVertices(grannyMesh, GrannyPN33VertexType, grannyVertData.data());

	int vertCount = grannyMesh->PrimaryVertexData->VertexCount;
	this->vertData = new char[vertCount * 32 + 128]; //extra padding
	{
		float* floatVertBuffer = (float*)vertData;
		for (int i = 0; i < grannyMesh->PrimaryVertexData->VertexCount; i++) {
			GrannyGetSingleVertex(grannyMesh->PrimaryVertexData, i, grannyMesh->PrimaryVertexData->VertexType, (void*)(vertData + 32 * i)); //does this fuck stuff up if it outputs to a too small buffer?
			EsoVert* vert = (EsoVert*)(vertData + 32 * i);
			vert->x = vert->x * -1;
			bool inverted = false;
			if (0 > vert->nx) {
				vert->nx = vert->nx + 32768;
				inverted = true;
			}
			if (0 > vert->ny) {
				vert->ny = vert->ny + 32768;
				inverted = true;
			}
			//vert->u = bx::halfFromFloat(vert->u / 1024.f);
			//vert->v = bx::halfFromFloat(vert->v / -1024.f);


			float fnormX = ((float)vert->nx) / -16384.f + 1.f;
			float fnormY = ((float)vert->ny) / 16384.f - 1.f;
			float fnormZ = std::sqrtf(1 - fnormX * fnormX - fnormY * fnormY);
			if (inverted) fnormZ *= -1;
			floatVertBuffer[i * 8 + 4] = fnormX;
			floatVertBuffer[i * 8 + 5] = fnormY;
			floatVertBuffer[i * 8 + 6] = fnormZ;

			//std::cout << "TEST " << vert->x << " " << vert->y << " " << vert->z << " " << fnormX << " " << fnormY  << " " << fnormZ << "\n";
		}
	}



	idxCount = grannyMesh->PrimaryTopology->Index16Count;
	idxData = std::vector<uint16_t>(idxCount);
	GrannyCopyMeshIndices(grannyMesh, 2, idxData.data());

	//for (int i = 0; i < grannyIdxData.size(); i += 3) {
	//	cout << grannyIdxData[i] << " " << grannyIdxData[i + 1] << " " << grannyIdxData[i + 2] << endl;
	//}
	GrannyFreeFile(file);

	BufferDescriptor vBufferDesc;
	vBufferDesc.size = vertCount * 32;
	vBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
	vBufferDesc.mappedAtCreation = false;
	vBufferDesc.label = "vertex buffer";
	this->vertBuffer = device.createBuffer(vBufferDesc);


	this->vertAttributes = std::vector<wgpu::VertexAttribute>(2);
	//position
	vertAttributes[0].shaderLocation = 0;
	vertAttributes[0].offset = 0;
	vertAttributes[0].format = VertexFormat::Float32x3;
	//color
	vertAttributes[1].shaderLocation = 1;
	vertAttributes[1].offset = 4 * sizeof(float);
	vertAttributes[1].format = VertexFormat::Float32x3;

	vertLayout;
	vertLayout.attributeCount = static_cast<uint32_t>(vertAttributes.size());
	vertLayout.attributes = vertAttributes.data();
	vertLayout.arrayStride = 32;
	vertLayout.stepMode = VertexStepMode::Vertex;

	queue.writeBuffer(vertBuffer, 0, vertData, vBufferDesc.size);


	//IDX BUFFER
	BufferDescriptor idxBufferDesc;
	//A writeBuffer operation must copy a number of bytes that is a multiple of 4. To ensure so we can switch bufferDesc.size for (bufferDesc.size + 3) & ~3.
	idxBufferDesc.size = (idxCount * sizeof(uint16_t) + 3) & ~3;
	idxBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
	idxBufferDesc.mappedAtCreation = false;
	idxBufferDesc.label = "idx buffer";
	this->idxBuffer = device.createBuffer(idxBufferDesc);
	queue.writeBuffer(idxBuffer, 0, idxData.data(), idxBufferDesc.size);

	this->vertBufferSize = vertCount * 32;
	this->idxBufferSize = (int)idxData.size() * sizeof(uint16_t);
	

}
