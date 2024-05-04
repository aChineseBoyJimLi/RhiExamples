#pragma once

#include <fstream>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>

#include "DirectXMesh.h"
#include "DirectXTex.h"
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

        const uint8_t*  GetData() const { return m_Data; }
        size_t          GetSize() const { return m_Size; }
        bool            IsEmpty() const { return m_Data == nullptr || m_Size == 0; }
        bool            ReadBinaryFile(const std::filesystem::path& path);
        void            Release();  

    private:
        uint8_t* m_Data;
        size_t m_Size;
    };
    
    class Mesh
    {
    public:
        Mesh() : m_Mesh(nullptr) {}
        ~Mesh() { Release(); }

        bool            IsEmpty() const { return m_Mesh == nullptr; }
        uint32_t        GetVerticesCount() const { return m_Mesh ? m_Mesh->mNumVertices : 0; }
        uint32_t        GetPrimCount() const { return m_Mesh ? m_Mesh->mNumFaces : 0; }
        uint32_t        GetIndicesCount() const { return (uint32_t)m_Indices.size(); }
        const aiMesh*   GetMesh() const { return m_Mesh; }
        aiMesh*         GetMesh() { return m_Mesh; }
        bool            ReadMesh(const std::filesystem::path& inPath);
        void            Release();
        bool            ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
                            , std::vector<uint8_t>& outUniqueVertexIndices
                            , std::vector<DirectX::MeshletTriangle>& outPackedPrimitiveIndices) const;

        bool            ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
                            , std::vector<uint8_t>& outUniqueVertexIndices
                            , std::vector<DirectX::MeshletTriangle>& outPackedPrimitiveIndices
                            , std::vector<DirectX::CullData>& outMeshletCullData) const;

        void            GetPositionData(std::vector<glm::vec4> &outPositions) const;
        
        void            GetTexCoord0Data(std::vector<glm::vec2> &outTexCoords) const;

        void            GetNormalData(std::vector<glm::vec4> &outNormals) const;

        const void*     GetPositionData() const { return m_Mesh ? m_Mesh->mVertices : nullptr; }
        size_t          GetPositionDataByteSize() const { return m_Mesh ? m_Mesh->mNumVertices * sizeof(aiVector3D) : 0; }
        const void*     GetTexCoordData() const {return m_Mesh && m_Mesh->HasTextureCoords(0) ? m_Mesh->mTextureCoords[0] : nullptr; }
        size_t          GetTexCoordDataByteSize() const { return m_Mesh ? m_Mesh->mNumVertices * sizeof(aiVector3D) : 0; }
        const void*     GetIndicesData() const { return m_Mesh ? m_Indices.data() : nullptr; }
        size_t          GetIndicesDataByteSize() const { return m_Mesh ? m_Indices.size() * sizeof(uint32_t) : 0; }
        const void*     GetNormalData() const { return m_Mesh && m_Mesh->HasNormals() ? m_Mesh->mNormals : nullptr; }
        size_t          GetNormalDataByteSize() const { return m_Mesh ? m_Mesh->mNumVertices * sizeof(aiVector3D) : 0; }
        
    private:
        Assimp::Importer m_Importer;
        aiMesh* m_Mesh;
        std::vector<uint32_t> m_Indices;
    };

    struct TextureSubresourceData
    {
        uint32_t RowPitch = 0;
        uint32_t DepthPitch = 0;
        uint32_t DataOffset = 0;
        uint32_t DataSize = 0;
    };

    class Texture
    {
    public:
        Texture(bool sRGB);
        ~Texture() { Release(); }
        const DirectX::TexMetadata& GetTextureDesc() const { return m_ScratchImage.GetMetadata(); }
        const DirectX::ScratchImage& GetScratchImage() const { return m_ScratchImage; }

        bool ReadTexture(const std::filesystem::path& path);
        void Release();

        bool IsEmpty() const { return m_ScratchImage.GetPixels() == nullptr; }
        
    private:
        DirectX::ScratchImage m_ScratchImage;
        DirectX::TexMetadata m_Metadata;
        bool m_sRGB;
    };
    
    std::shared_ptr<Blob>           LoadShaderImmediately(const char* inShaderName);
    std::shared_ptr<Mesh>           LoadMeshImmediately(const char* inMeshName);
    std::shared_ptr<Texture>        LoadTextureImmediately(const char* inTextureName, bool sRGB = false);
    
    void                            ChangeShaderPath(const char* inPath);
    const std::filesystem::path&    GetShaderPath();
}
