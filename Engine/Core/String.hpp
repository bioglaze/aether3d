#ifndef STRING_H
#define STRING_H

namespace ae3d
{
    /**
      Constant-length string.
     */
    template< int Size >
    class String
    {
    public:
        String();
        explicit String( const char* text );
        bool operator<( const String< Size >& /*other*/ ) const { return false; }
        bool operator==( const char* other ) const;
        const char* CStr() const;

    private:
        char data[ Size ];
    };
    
    template< int Size >
    String< Size >::String()
    {
        data[ 0 ] = 0;
    }
    
    template< int Size >
    String< Size >::String( const char* text )
    {
        while ((*data++ = *text++));
    }
    
    template< int Size >
    const char* String< Size >::CStr() const
    {
        return data;
    }

    template< int Size >
    bool String< Size >::operator==( const char* other ) const
    {
        while (*data == *other)
        {
            if(*data == '\0')
            {
                return true;
            }

            data++;
            other++;
        }
        
        return *data - *other == 0;
    }
    
    using String16 = ae3d::String< 16 >;
    using String32 = ae3d::String< 32 >;
    using String64 = ae3d::String< 64 >;
    using String128 = ae3d::String< 128 >;
}

#endif
