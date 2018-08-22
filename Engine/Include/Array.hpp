#pragma once

#include <string.h>

template< typename T > struct Array
{
    Array() {}
    
    Array( int elementCount ) { Allocate( elementCount );  }

    Array( const Array<T>& other ) { *this = other; }

    ~Array()
    {
        delete[] elements;
        elements = nullptr;
        count = 0;
    }
    
    Array<T>& operator=( const Array<T>& other )
    {
        if (this == &other)
        {
            return *this;
        }
        
        delete[] elements;
        count = other.count;
        elements = new T[ count ];
        memcpy( elements, other.elements, count * sizeof( T ) );
        return *this;
    }
    
    T& operator[]( int index ) { return elements[ index ]; }

    const T& operator[]( int index ) const { return elements[ index ]; }
    
    void Add( const T& item )
    {
        T* after = new T[ count + 1 ]();
        memcpy( after, elements, count * sizeof( T ) );
        
        after[ count ] = item;
        delete[] elements;
        elements = after;
        count = count + 1;
    }
    
    void Allocate( int size )
    {
        delete[] elements;
        elements = size > 0 ? new T[ size ]() : nullptr;
        count = size;
    }
    
    T* elements = nullptr;
    int count = 0;
};

