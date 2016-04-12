#ifndef MACROS_H
#define MACROS_H

#if (defined( _MSC_VER ) && (_MSC_VER >= 1900)) || defined(__APPLE__) || defined(__GNUC__)
#define ALIGNAS( n ) alignas( n )
#else
#define ALIGNAS( n ) __declspec( align( n ) )
#endif

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }
#define AE3D_CHECK_D3D(x, msg) if (x != S_OK) { ae3d::System::Assert( false, msg ); }
#define AE3D_CHECK_VULKAN(x, msg) if (x != VK_SUCCESS) { ae3d::System::Assert( false, msg ); }

#endif
