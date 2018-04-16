#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#if RENDERER_D3D12
#include <d3d12.h>
#endif
#if RENDERER_METAL
#import <Metal/Metal.h>
#endif
#if RENDERER_VULKAN
#include <vulkan/vulkan.h>
#endif
#include "Vec3.hpp"

namespace ae3d
{
    /// Contains a vertex and index buffer. Indices are 16-bit.
    class VertexBuffer
    {
    public:
        enum class VertexFormat { PTC, PTN, PTNTC, PTNTC_Skinned, Empty };

        /// Triangle of 3 vertices.
        struct Face
        {
            Face() : a(0), b(0), c(0) {}
            
            Face( unsigned short fa, unsigned short fb, unsigned short fc )
            : a( fa )
            , b( fb )
            , c( fc )
            {}
            
            unsigned short a, b, c;
        };

        /// Vertex with position, texture coordinate and color.
        struct VertexPTC
        {
            VertexPTC() {}
            VertexPTC( const Vec3& pos, float aU, float aV )
            : position( pos )
            , u( aU )
            , v( aV )
            {
            }
            
            Vec3 position;
            float u = 0, v = 0;
            Vec4 color = { 1, 1, 1, 1 };
        };

        /// Vertex with position, texcoord, normal, tangent (handedness in .w) and color.
        struct VertexPTNTC
        {
            Vec3 position;
            float u, v;
            Vec3 normal;
            Vec4 tangent;
            Vec4 color;
        };

        struct VertexPTNTC_Skinned
        {
            Vec3 position;
            float u, v;
            Vec3 normal;
            Vec4 tangent;
            Vec4 color;
            ae3d::Vec4 weights;
            int bones[ 4 ];
        };
        
        /// Vertex with position, texcoord and normal.
        struct VertexPTN
        {
            Vec3 position;
            float u, v;
            Vec3 normal;
        };

#if RENDERER_VULKAN
		VertexBuffer() : bindingDescriptions(), attributeDescriptions() {}
#endif

#if RENDERER_D3D12
        /// Return Stride in bytes.
        unsigned GetStride() const;

        /// \return Index buffer size in bytes.
        unsigned GetIBSize() const;

        /// \return Vertex buffer resource.
        ID3D12Resource* GetVBResource() { return vb; }

        /// \return Index buffer offset from the beginning of the vb.
        long GetIBOffset() const { return ibOffset; }

        /// \return VB view
        const D3D12_VERTEX_BUFFER_VIEW* GetView() const { return &vertexBufferView; }

        /// \return IB view
        const D3D12_INDEX_BUFFER_VIEW* GetIndexView() const { return &indexBufferView; }
#endif

        /// Binds the buffer. Must be called before GfxDevice::Draw.
        void Bind() const;

        /// \return Face count.
        int GetFaceCount() const { return elementCount; }

        VertexFormat GetVertexFormat() const { return vertexFormat; }

        /// \return True if the buffer contains geometry ready for rendering.
        bool IsGenerated() const { return elementCount != 0; }

        /// Generates the buffer from supplied geometry.
        /// \param faces Faces.
        /// \param faceCount Face count.
        /// \param vertices Vertices.
        /// \param vertexCount Vertex count.
        void Generate( const Face* faces, int faceCount, const VertexPTC* vertices, int vertexCount );

        /// Generates the buffer from supplied geometry.
        /// \param faces Faces.
        /// \param faceCount Face count.
        /// \param vertices Vertices.
        /// \param vertexCount Vertex count.
        void Generate( const Face* faces, int faceCount, const VertexPTN* vertices, int vertexCount );

        /// Generates the buffer from supplied geometry.
        /// \param faces Faces.
        /// \param faceCount Face count.
        /// \param vertices Vertices.
        /// \param vertexCount Vertex count.
        void Generate( const Face* faces, int faceCount, const VertexPTNTC* vertices, int vertexCount );

        /// Generates the buffer from supplied geometry.
        /// \param faces Faces.
        /// \param faceCount Face count.
        /// \param vertices Vertices.
        /// \param vertexCount Vertex count.
        void Generate( const Face* faces, int faceCount, const VertexPTNTC_Skinned* vertices, int vertexCount );

        /// Sets a graphics API debug name for the buffer, visible in debugging tools. Must be called after Generate().
        /// \param name Name
        void SetDebugName( const char* name );

#if RENDERER_METAL
        id<MTLBuffer> GetVertexBuffer() const { return vertexBuffer; }
        id<MTLBuffer> GetIndexBuffer() const { return indexBuffer; }
#if 1
        id<MTLBuffer> positionBuffer;
        id<MTLBuffer> texcoordBuffer;
        id<MTLBuffer> colorBuffer;
        id<MTLBuffer> normalBuffer;
        id<MTLBuffer> tangentBuffer;
        id<MTLBuffer> boneBuffer;
        id<MTLBuffer> weightBuffer;
#endif
        
#endif
#if RENDERER_VULKAN
        static const unsigned VERTEX_BUFFER_BIND_ID = 0;

        VkPipelineVertexInputStateCreateInfo* GetInputState() { return &inputStateCreateInfo; }
        VkBuffer* GetVertexBuffer() { return &vertexBuffer; }
        VkBuffer* GetIndexBuffer() { return &indexBuffer; }

#endif
        /// Destroys graphics API objects.
        static void DestroyBuffers();

        static const int posChannel = 0;
        static const int uvChannel = 1;
        static const int colorChannel = 2;
        static const int normalChannel = 3;
        static const int tangentChannel = 4;
        static const int boneChannel = 5;
        static const int weightChannel = 6;

    private:

#if RENDERER_D3D12
        void UploadVB( void* faces, void* vertices, unsigned ibSize );
        // Index buffer is stored in the vertex buffer after vertex data.
        ID3D12Resource* vb = nullptr;
        D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
        D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
        long ibOffset = 0;
        int sizeBytes = 0;
#endif
        int elementCount = 0;
        VertexFormat vertexFormat = VertexFormat::PTC;
#if RENDERER_OPENGL
        unsigned vaoId = 0;
        unsigned vboId = 0;
        unsigned iboId = 0;
#endif
#if RENDERER_METAL
        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> indexBuffer;
#endif
#if RENDERER_VULKAN
        void GenerateVertexBuffer( const void* vertexData, int vertexBufferSize, int vertexStride, const void* indexData, int indexBufferSize );
        void CreateInputState( int vertexStride );

        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexMem = VK_NULL_HANDLE;
        VkPipelineVertexInputStateCreateInfo inputStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO, nullptr, 0, 0, nullptr, 0, nullptr };
        VkVertexInputBindingDescription bindingDescriptions;
        VkVertexInputAttributeDescription attributeDescriptions[ 7 ];

        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexMem = VK_NULL_HANDLE;
#endif
    };
}
#endif
