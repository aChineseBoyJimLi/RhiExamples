#include "AssetsManager.h"

namespace AssetsManager
{
    static std::filesystem::path s_ShaderPath = std::filesystem::current_path() / ".." / "Shaders";
    static std::filesystem::path s_ModelPath = std::filesystem::current_path() / ".." / ".." / "Assets" / "Models";
    static std::filesystem::path s_TexturePath = std::filesystem::current_path() / ".." / ".." / "Assets" / "Textures";
    
    bool Blob::ReadBinaryFile(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if(!file.is_open())
        {
            Log::Error("File %s dose not exits or is locked", path.string().c_str());
            return nullptr;
        }

        file.seekg(0, std::ios::end);
        m_Size = file.tellg();
        file.seekg(0, std::ios::beg);

        m_Data = static_cast<char*>(malloc(m_Size));
        if (m_Data == nullptr)
        {
            Log::Error("Failed to malloc memory for the file %s", path.string().c_str());
            m_Data = nullptr;
            m_Size = 0;
            return false;
        }

        file.read(m_Data, m_Size);

        if (!file.good())
        {
            Log::Error("Read file %s error", path.string().c_str());
            m_Data = nullptr;
            m_Size = 0;
            return false;
        }

        return true;
    }

    void Blob::Release()
    {
        if(m_Data)
        {
            free(m_Data);
            m_Data = nullptr;
        }
        m_Size = 0;
    }

    bool Mesh::ReadMesh(const std::filesystem::path& inPath)
    {
        const aiScene* scene = m_Importer.ReadFile(inPath.generic_string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Log::Error("Failed to load model %s", inPath.string().c_str());
            return false;
        }

        m_Mesh = scene->mMeshes[0];
        
        return true;
    }

    void Mesh::Release()
    {
        if(m_Mesh)
        {
            m_Importer.FreeScene();
            m_Mesh = nullptr;
        }
    }

    bool Mesh::ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
            , std::vector<uint8_t>& outUniqueVertexIndices
            , std::vector<DirectX::MeshletTriangle>& outPackedPrimitiveIndices) const
    {
        if(m_Mesh == nullptr || m_Mesh->mNumVertices == 0 || m_Mesh->mNumFaces == 0)
        {
            Log::Error("Mesh is empty");
            return false;
        }
        
        std::vector<DirectX::XMFLOAT3> vertices(m_Mesh->mNumVertices);
        for(uint32_t i = 0; i < m_Mesh->mNumVertices; i++)
        {
            vertices[i] = DirectX::XMFLOAT3(m_Mesh->mVertices[i].x, m_Mesh->mVertices[i].y, m_Mesh->mVertices[i].z);
        }
        
        std::vector<uint32_t> indices;
        for(uint32_t i = 0; i < m_Mesh->mNumFaces; i++)
        {
            const aiFace& Face = m_Mesh->mFaces[i];
            for(uint32_t j = 0; j < Face.mNumIndices; j++)
            {
                indices.push_back(Face.mIndices[j]);
            }
        }
        
        HRESULT hr = DirectX::ComputeMeshlets(indices.data()
            , m_Mesh->mNumFaces
            , vertices.data()
            , vertices.size()
            , nullptr
            , outMeshlets
            , outUniqueVertexIndices
            , outPackedPrimitiveIndices);

        if(FAILED(hr))
        {
            Log::Error("Failed to compute meshlets");
            return false;
        }

        return true;
    }
    
    std::shared_ptr<Blob> LoadShaderImmediately(const char* inShaderName)
    {
        const std::filesystem::path path = s_ShaderPath / inShaderName;
        std::shared_ptr<Blob> blob = std::make_shared<Blob>();
        if(!blob->ReadBinaryFile(path))
        {
            return nullptr;
        }
        return blob;
    }

    std::shared_ptr<Mesh> LoadMeshImmediately(const char* inMeshName)
    {
        const std::filesystem::path path = s_ModelPath / inMeshName;
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
        if(!mesh->ReadMesh(path))
        {
            return nullptr;
        }
        return mesh;
    }

    std::shared_ptr<Texture> LoadTextureImmediately(const char* inTextureName)
    {
        const std::filesystem::path path = s_TexturePath / inTextureName;
        std::shared_ptr<Texture> texture = std::make_shared<Texture>();

        return texture;
    }

    void ChangeShaderPath(const char* inPath)
    {
        s_ShaderPath = std::filesystem::path(inPath);
    }

    const std::filesystem::path& GetShaderPath()
    {
        return s_ShaderPath;
    }
}
