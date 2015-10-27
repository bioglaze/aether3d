#ifndef FONT_H
#define FONT_H

namespace ae3d
{
    namespace FileSystem
    {
        struct FileContentsData;
    }
    
    /// Contains glyphs loaded from AngelCode BMFont files. For Mac there is a compatible program called BMGlyph.
    class Font
    {
      public:
        /// Constructor.
        Font();
        
        /**
          \param fontTex Font texture. No outline support.
          \param metaData BMFont metadata. Must be text or binary.
         */
        void LoadBMFont( const class Texture2D* fontTex, const FileSystem::FileContentsData& metaData );
        
        /** \return Font texture. */
        const Texture2D* GetTexture() const { return texture; }
        
        /// \return Text width.
        int TextWidth( const char* text ) const;
        
      private:
        friend class TextRendererComponent;

        struct Character
        {
            float x = 0, y = 0;
            float width = 0, height = 0;
            float xOffset = 0, yOffset = 0;
            float xAdvance = 0;
        };

        /**
         \param text Text.
         \param color Color in range 0-1.
         \param outVertexBuffer Vertex buffer created from text.
         */
        void CreateVertexBuffer( const char* text, const struct Vec4& color, class VertexBuffer& outVertexBuffer ) const;

        /** \param metaData BMFont text metadata. */
        void LoadBMFontMetaText(const FileSystem::FileContentsData& metaData);
        
        /** \param metaData BMFont binary metadata. */
        void LoadBMFontMetaBinary(const FileSystem::FileContentsData& metaData);
        
        /** The spacing for each character (horizontal, vertical). */
        int spacing[ 2 ];
        
        /** The padding for each character (up, right, down, left). */
        int padding[ 4 ];
        
        /** This is the distance in pixels between each line of text. */
        int lineHeight = 32;
        
        /** The number of pixels from the absolute top of the line to the base of the characters. */
        int base = 32;
        
        Character chars[ 256 ];
        const Texture2D* texture = nullptr;
    };
}

#endif
