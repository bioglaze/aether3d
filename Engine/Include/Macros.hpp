#ifndef MACROS_H
#define MACROS_H

// Alignment for SIMD.
#if defined( _MSC_VER ) || defined( COMPILER_INTEL )
#       define BEGIN_ALIGNED( n, a )                            __declspec( align( a ) ) n
#       define END_ALIGNED( n, a )
#elif defined( __GNUC__ ) || defined( __clang__ )
#       define BEGIN_ALIGNED( n, a )                            n
#       define END_ALIGNED( n, a )                              __attribute__( (aligned( a ) ) )
#endif

// TODO comments as compile warnings/links
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