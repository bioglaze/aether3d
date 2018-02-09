#include "ComputeShader.hpp"

// Unimplemented because OpenGL renderer is 4.1. If Apple updates their GL driver to support 4.3 these will get implemented.

void ae3d::ComputeShader::Load( const char* /*source*/ )
{
}

void ae3d::ComputeShader::Load( const char* /*metalShaderName*/, const FileSystem::FileContentsData& /*dataHLSL*/, const FileSystem::FileContentsData& /*dataSPIRV*/ )
{

}

void ae3d::ComputeShader::Dispatch( unsigned /*groupCountX*/, unsigned /*groupCountY*/, unsigned /*groupCountZ*/ )
{

}
