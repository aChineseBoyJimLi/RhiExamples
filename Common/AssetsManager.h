#pragma once


#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "DirectXMesh.h"
#include "Log.h"

namespace AssetsManager
{
    class Blob
    {
    public:
        Blob() : m_Data(nullptr), m_Size(0) {}
        ~Blob() { Release(); }
        Blob(const Blob&) = delete;
        Blob(Blob&&) = delete;
        Blob& operator=(const Blob&) = delete;
        Blob& operator=(Blob&&) = delete;

        const char*     GetData() const { return m_Data; }
        size_t          GetSize() const { return m_Size; }
        bool            IsEmpty() const { return m_Data == nullptr || m_Size == 0; }
        bool            ReadBinaryFile(const std::filesystem::path& path);
        void            Release();  

    private:
        char* m_Data;
        size_t m_Size;
    };

    class Mesh
    {
    public:
        Mesh() : m_Mesh(nullptr) {}
        ~Mesh() { Release(); }

        uint32_t GetVertexCount() const { return m_Mesh ? m_Mesh->mNumVertices : 0; }
        uint32_t GetPrimCount() const { return m_Mesh ? m_Mesh->mNumFaces : 0; }
        const aiMesh* GetMesh() const { return m_Mesh; }
        aiMesh* GetMesh() { return m_Mesh; }

        bool ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
            , std::vector<uint8_t>& outUniqueVertexIndices
            , std::vector<DirectX::MeshletTriangle>& primitiveIndices) const;
        
        bool ReadMesh(const std::filesystem::path& inPath);
        void Release();
        
    private:
        Assimp::Importer m_Importer;
        aiMesh* m_Mesh;
    };
    
    std::shared_ptr<Blob>           LoadShaderImmediately(const char* inShaderName);
    std::shared_ptr<Mesh>           LoadMeshImmediately(const char* inMeshName);
    
    void                            ChangeShaderPath(const char* inPath);
    const std::filesystem::path&    GetShaderPath();
}
