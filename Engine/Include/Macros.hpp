#ifndef MACROS_H
#define MACROS_H

#define AE3D_SAFE_RELEASE(x) if (x) { x->Release(); x = nullptr; }
#define AE3D_CHECK_D3D(x, msg) if (x != S_OK) { ae3d::System::Assert( false, msg ); }
#define AE3D_CHECK_VULKAN(x, msg) if (x != VK_SUCCESS) { ae3d::System::Assert( false, msg ); }

#endif
