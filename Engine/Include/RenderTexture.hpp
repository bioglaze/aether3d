#ifndef RENDER_TEXTURE_H
#define RENDER_TEXTURE_H

#include "TextureBase.hpp"
#include "Vec3.hpp"

namespace ae3d
{
    /// 2D render texture.
    class RenderTexture2D : public TextureBase
    {
  public:
        /// \param width Width.
        /// \param height Height.
        /// \param wrap Wrapping mode.
        /// \param filter Filtering mode.
        void Create( int width, int height, TextureWrap wrap, TextureFilter filter );
        
        /// \return FBO.
        unsigned GetFBO() const { return fboId; }
        
  private:
        unsigned rboId = 0;
        unsigned fboId = 0;
    };
}
#endif
