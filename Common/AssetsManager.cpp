#include "AssetsManager.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define OUTPUT_PARSE_FAILED_RESULT  std::string message = std::system_category().message(hr);\
            Log::Error("Failed to parse the image: %s, ", path.string().c_str());\
            Log::Error(message.c_str());\

#define OUTPUT_LOAD_FAILED_RESULT  std::string message = std::system_category().message(hr);\
            Log::Error("Failed to load the image: %s, ", path.string().c_str());\
            Log::Error(message.c_str());\

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

        m_Data = static_cast<uint8_t*>(malloc(m_Size));
        if (m_Data == nullptr)
        {
            Log::Error("Failed to malloc memory for the file %s", path.string().c_str());
            m_Data = nullptr;
            m_Size = 0;
            return false;
        }

        file.read(reinterpret_cast<char*>(m_Data), m_Size);

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
        std::shared_ptr<Blob> blob = std::make_shared<Blob>();
        if(!blob->ReadBinaryFile(inPath))
            return false;
        
        const aiScene* scene = m_Importer.ReadFileFromMemory(blob->GetData(), blob->GetSize(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_GenBoundingBoxes);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            Log::Error("Failed to load model %s", inPath.string().c_str());
            return false;
        }

        m_Mesh = scene->mMeshes[0];

        if(m_Mesh && m_Mesh->mNumFaces)
        {
            for(uint32_t i = 0; i < m_Mesh->mNumFaces; i++)
            {
                const aiFace& Face = m_Mesh->mFaces[i];
                for(uint32_t j = 0; j < Face.mNumIndices; j++)
                {
                    m_Indices.push_back(Face.mIndices[j]);
                }
            }
        }
        
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

    bool Mesh::ComputeMeshlets(std::vector<DirectX::Meshlet>& outMeshlets
                            , std::vector<uint8_t>& outUniqueVertexIndices
                            , std::vector<DirectX::MeshletTriangle>& outPackedPrimitiveIndices
                            , std::vector<DirectX::CullData>& outMeshletCullData) const
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

        outMeshletCullData.resize(outMeshlets.size());
        
        hr = DirectX::ComputeCullData(vertices.data()
            , vertices.size()
            , outMeshlets.data()
            , outMeshlets.size()
            , reinterpret_cast<const uint32_t*>(outUniqueVertexIndices.data()) // index buffer is uint32_t, so cast uniqueVertexIndices to uint32_t*
            , outUniqueVertexIndices.size() / 4
            , outPackedPrimitiveIndices.data()
            , outPackedPrimitiveIndices.size()
            , outMeshletCullData.data());

        if(FAILED(hr))
        {
            Log::Error("Failed to compute meshlets cull data");
            return false;
        }
        
        return true;
    }

    void Mesh::GetPositionData(std::vector<glm::vec4> &outPositions) const
    {
        if(m_Mesh == nullptr)
            return ;
        outPositions.resize(m_Mesh->mNumVertices);
        for(uint32_t i = 0; i < m_Mesh->mNumVertices; i++)
        {
            outPositions[i] = glm::vec4(m_Mesh->mVertices[i].x, m_Mesh->mVertices[i].y, m_Mesh->mVertices[i].z, 1.0f);
        }
    }
        
    void Mesh::GetTexCoord0Data(std::vector<glm::vec2> &outTexCoords) const
    {
        if(m_Mesh == nullptr || !m_Mesh->HasTextureCoords(0))
            return ;
        outTexCoords.resize(m_Mesh->mNumVertices);
        for(uint32_t i = 0; i < m_Mesh->mNumVertices; i++)
        {
            outTexCoords[i] = glm::vec2(m_Mesh->mTextureCoords[0][i].x, m_Mesh->mTextureCoords[0][i].y);
        }
    }

    void Mesh::GetNormalData(std::vector<glm::vec4> &outNormals) const
    {
        if(m_Mesh == nullptr || !m_Mesh->HasNormals())
            return ;
        outNormals.resize(m_Mesh->mNumVertices);
        for(uint32_t i = 0; i < m_Mesh->mNumVertices; i++)
        {
            outNormals[i] = glm::vec4(m_Mesh->mNormals[i].x, m_Mesh->mNormals[i].y, m_Mesh->mNormals[i].z, 0);
        }
    }

    Texture::Texture(bool sRGB)
        : m_sRGB(sRGB)
    {
        
    }

    bool Texture::ReadTexture(const std::filesystem::path& path)
    {
        std::shared_ptr<Blob> blob = std::make_shared<Blob>();
        if(!blob->ReadBinaryFile(path))
        {
            return false;
        }

        const std::filesystem::path extension = path.extension();
        
        if(extension.compare(".dds") == 0)
        {
            HRESULT hr = DirectX::GetMetadataFromDDSMemory(blob->GetData(), blob->GetSize(), DirectX::DDS_FLAGS_NONE, m_Metadata);
            if(FAILED(hr))
            {
                OUTPUT_PARSE_FAILED_RESULT
                return false;
            }
            hr = DirectX::LoadFromDDSMemory(blob->GetData(), blob->GetSize(), DirectX::DDS_FLAGS_NONE, nullptr, m_ScratchImage);
            if(FAILED(hr))
            {
                OUTPUT_LOAD_FAILED_RESULT
                return false;
            }
        }
        else if(extension.compare(".tga") == 0)
        {
            HRESULT hr = DirectX::GetMetadataFromTGAMemory(blob->GetData(), blob->GetSize(), m_Metadata);
            if(FAILED(hr))
            {
                OUTPUT_PARSE_FAILED_RESULT
                return false;
            }
            hr = DirectX::LoadFromTGAMemory(blob->GetData(), blob->GetSize(), DirectX::TGA_FLAGS_NONE, nullptr, m_ScratchImage);
            if(FAILED(hr))
            {
                OUTPUT_LOAD_FAILED_RESULT
                return false;
            }
        }
        else if(extension.compare(".hdr") == 0)
        {
            HRESULT hr = DirectX::GetMetadataFromHDRMemory(blob->GetData(), blob->GetSize(), m_Metadata);
            if(FAILED(hr))
            {
                OUTPUT_PARSE_FAILED_RESULT
                return false;
            }

            hr = DirectX::LoadFromHDRMemory(blob->GetData(), blob->GetSize(), nullptr, m_ScratchImage);
            if(FAILED(hr))
            {
                OUTPUT_LOAD_FAILED_RESULT
                return false;
            }
        }
        else if(extension.compare(".png") == 0 || extension.compare(".jpg") == 0)
        {
            int width, height, originalChannels, channels;
            if(!stbi_info_from_memory(blob->GetData()
                , static_cast<int>(blob->GetSize())
                , &width
                , &height
                , &originalChannels))
            {
                Log::Error("Failed to parse the image: %s, ", path.string().c_str());
                return false;
            }
            m_Metadata.width = width;
            m_Metadata.height = height;
            m_Metadata.depth = 1;
            m_Metadata.arraySize = 1;
            m_Metadata.mipLevels = 1;
            m_Metadata.format = m_sRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
            m_Metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;
            m_Metadata.miscFlags = 0;
            m_Metadata.miscFlags2 = originalChannels == 3 ? DirectX::TEX_ALPHA_MODE_OPAQUE : 0;

            channels = originalChannels == 3 ? 4 : originalChannels;
            
            stbi_uc* bitmap = stbi_load_from_memory( blob->GetData()
                                ,static_cast<int>(blob->GetSize())
                                , &width
                                , &height
                                , &originalChannels, channels);

            if(bitmap == nullptr)
            {
                Log::Error("Failed to load the image: %s, ", path.string().c_str());
                return false;
            }
            
            // m_ScratchImage.Initialize(m_Metadata);
            DirectX::Image image;
            image.width = m_Metadata.width;
            image.height = m_Metadata.height;
            image.format = m_Metadata.format;
            image.rowPitch = m_Metadata.width * channels;
            image.slicePitch = image.rowPitch * m_Metadata.height;
            image.pixels = bitmap;
            m_ScratchImage.InitializeFromImage(image);
        }
        else
        {
            Log::Error("Texture format %s is not supported", extension.string().c_str());
            return false;
        }
        
        return true;
    }

    void Texture::Release()
    {
        
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

    std::shared_ptr<Texture> LoadTextureImmediately(const char* inTextureName, bool sRGB)
    {
        const std::filesystem::path path = s_TexturePath / inTextureName;
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(sRGB);
        if(!texture->ReadTexture(path))
        {
            return nullptr;
        }
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
