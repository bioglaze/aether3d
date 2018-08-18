#pragma once

#include <string.h>

template< typename T > struct Array
{
    ~Array()
    {
        delete[] elements;
        elements = nullptr;
        elementCount = 0;
    }
    
    T& operator=( const T& other )
    {
        delete[] elements;
        elementCount = other.elementCount;
        elements = new T[ elementCount ];
        memcpy( elements, other.elements, elementCount * sizeof( T ) );
    }
    
    T& operator[]( int index ) const
    {
        return elements[ index ];
    }
    
    int GetLength() const noexcept
    {
        return elementCount;
    }
    
    void Add( const T& item )
    {
        T* after = new T[ elementCount + 1 ];
        memcpy( after, elements, elementCount * sizeof( T ) );
        
        after[ elementCount ] = item;
        delete[] elements;
        elements = after;
        elementCount = elementCount + 1;
    }
    
    void Allocate( int size )
    {
        delete[] elements;
        elements = size > 0 ? new T[ size ] : nullptr;
        elementCount = size;
    }
    
    T* elements = nullptr;
    int elementCount = 0;
};

