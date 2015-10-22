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
        
        /// \return FBO.
        unsigned GetFBO() const { return fboId; }
        
  private:
        unsigned rboId = 0;
        unsigned fboId = 0;
        bool isCube = false;
    };
}
#endif
