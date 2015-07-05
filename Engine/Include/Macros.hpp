#ifndef MACROS_H
#define MACROS_H

#if (defined( _MSC_VER ) && (_MSC_VER >= 1900)) || defined(__APPLE__) || defined(__GNUC__)
#define ALIGNAS( n ) alignas( n )
#else
#define ALIGNAS( n ) __declspec( align( n ) )
#endif

// Prints TODO comments as compile warnings/links
#if _MSC_VER
#define Stringize( L ) #L
#define MakeString( M, L ) M(L)
#define $Line \
MakeString( Stringize, __LINE__ )
#define Todo \
__FILE__ "(" $Line "): TODO: "
// Usage: #pragma message(Todo "text")
#define A_TODO(msg) __pragma( message(Todo msg) )
#else
#define PRAGMA_MESSAGE(x) _Pragma(#x)
#define A_TODO(msg) PRAGMA_MESSAGE(message msg)
#endif

#endif
