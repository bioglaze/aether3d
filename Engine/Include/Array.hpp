// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once

template< typename T > struct Array
{
    Array() noexcept {}
    
    explicit Array( unsigned elementCount ) { Allocate( elementCount );  }

    Array( const Array<T>& other ) { *this = other; }

    ~Array() noexcept
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

        for ( unsigned i = 0; i < count; ++i)
        {
            elements[ i ] = other.elements[ i ];
        }

        return *this;
    }
    
    T& operator[]( unsigned index ) { return elements[ index ]; }

    const T& operator[]( unsigned index ) const { return elements[ index ]; }
    
    void Add( const T& item )
    {
        T* after = new T[ count + 1 ]();
        
        for ( unsigned i = 0; i < count; ++i)
        {
            after[ i ] = elements[ i ];
        }

        after[ count ] = item;
        delete[] elements;
        elements = after;
        count = count + 1;
    }
    
    void Remove( unsigned index )
    {
        for (unsigned i = index; i < count - 1 && count > 0; ++i)
        {
            elements[ i ] = elements[ i + 1 ];
        }

        if (count > 0)
        {
            --count;
        }
    }

    void Allocate( unsigned size )
    {
        if (count == size)
        {
            return;
        }
        
        delete[] elements;
        elements = size > 0 ? new T[ size ]() : nullptr;
        count = size;
    }
    
    T* elements = nullptr;
	unsigned count = 0;
};

