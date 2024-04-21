#pragma once


#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "DirectXMesh.h"
#include "Enum.h"
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
        bool ReadMesh(const std::filesystem::path& inPath);
        void Release();
        bool ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
            , std::vector<uint8_t>& outUniqueVertexIndices
            , std::vector<DirectX::MeshletTriangle>& outPackedPrimitiveIndices) const;
        
    private:
        Assimp::Importer m_Importer;
        aiMesh* m_Mesh;
    };

    struct TextureDesc
    {
        uint32_t Width;
        uint32_t Height;
        uint32_t Depth;
        uint32_t ArraySize;
        uint32_t MipLevels;
        EFormat  Format;
    };

    class Texture
    {
    public:
        const TextureDesc& GetTextureDesc() const { return m_TextureDesc; }
        void Release();
    private:
        TextureDesc m_TextureDesc;
    };
    
    std::shared_ptr<Blob>           LoadShaderImmediately(const char* inShaderName);
    std::shared_ptr<Mesh>           LoadMeshImmediately(const char* inMeshName);
    std::shared_ptr<Texture>        LoadTextureImmediately(const char* inTextureName);
    
    void                            ChangeShaderPath(const char* inPath);
    const std::filesystem::path&    GetShaderPath();
}
