#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include "TextureBase.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    /// Render texture.
    class RenderTexture : public TextureBase
    {
  public:
        /// Data type.
        enum class DataType
        {
            UByte,
            Float
        };

        /// Destroys graphics API objects.
        static void DestroyTextures();

        /// \param width Width.
        /// \param height Height.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        void Create2D( int width, int height, DataType dataType, TextureWrap wrap, TextureFilter filter );

        /// \param dimension Dimension.
        /// \param dataType Data type.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        void CreateCube( int dimension, DataType dataType, TextureWrap wrap, TextureFilter filter );

        /// \return True, if the texture is a cube map.
        bool IsCube() const { return isCube; }
        
#if RENDERER_OPENGL
        /// \return FBO.
        unsigned GetFBO() const { return fboId; }
#endif

  private:
#if RENDERER_OPENGL
        unsigned rboId = 0;
        unsigned fboId = 0;
#endif
        bool isCube = false;
    };
}
#endif
