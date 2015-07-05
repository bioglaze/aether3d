#ifndef VERTEX_BUFFER_H
#define VERTEX_BUFFER_H

#if AETHER3D_IOS
#import <Metal/Metal.h>
#endif
#include "Vec3.hpp"

namespace ae3d
{
    /// Contains geometry used for rendering.
    class VertexBuffer
    {
    public:
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
            float u, v;
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

        /// Vertex with position, texcoord and normal.
        struct VertexPTN
        {
            Vec3 position;
            float u, v;
            Vec3 normal;
            Vec4 tangent;
            Vec4 color;
        };

        /// Binds the buffer. Must be called before Draw or DrawRange.
        void Bind() const;

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
        
        /// Draws the whole buffer.
        void Draw() const;
        
        /// Draws a range of triangles.
        /// \param start Start index.
        /// \param end End index.
        void DrawRange( int start, int end ) const;

    private:
        enum class VertexFormat { PTC, PTN, PTNTC };
        static const int posChannel = 0;
        static const int uvChannel = 1;
        static const int colorChannel = 2;
        static const int normalChannel = 3;
        static const int tangentChannel = 4;
        
        unsigned vaoId = 0;
        unsigned vboId = 0;
        unsigned iboId = 0;
        
        int elementCount = 0;
        VertexFormat vertexFormat = VertexFormat::PTC;
#if AETHER3D_IOS
        id<MTLBuffer> vertexBuffer;
        id<MTLBuffer> indexBuffer;
#endif
    };
}
#endif
