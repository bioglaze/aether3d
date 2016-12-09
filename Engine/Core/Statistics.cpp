#include "Statistics.hpp"
#include <chrono>

namespace Statistics
{
    int drawCalls = 0;
    int barrierCalls = 0;
    int fenceCalls = 0;
    int textureBinds = 0;
    int shaderBinds = 0;
    int renderTargetBinds = 0;
    int vertexBufferBinds = 0;
    int createConstantBufferCalls = 0;
    int allocCalls = 0;
    int triangleCount = 0;
    float depthNormalsTimeMS = 0;
    float shadowMapTimeMS = 0;
    float frameTimeMS = 0;
    std::chrono::time_point< std::chrono::high_resolution_clock > startFrameTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startShadowMapTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startDepthNormalsTimePoint;
}

void Statistics::IncTriangleCount( int triangles )
{
    triangleCount += triangles;
}

int Statistics::GetTriangleCount()
{
    return triangleCount;
}

void Statistics::BeginShadowMapProfiling()
{
    Statistics::startShadowMapTimePoint = std::chrono::high_resolution_clock::now();
}

void Statistics::EndShadowMapProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startShadowMapTimePoint ).count();
    Statistics::shadowMapTimeMS = static_cast< float >(tDiff);
}

void Statistics::BeginDepthNormalsProfiling()
{
    Statistics::startDepthNormalsTimePoint = std::chrono::high_resolution_clock::now();
}

void Statistics::EndDepthNormalsProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startDepthNormalsTimePoint ).count();
    Statistics::depthNormalsTimeMS = static_cast< float >(tDiff);
}

void Statistics::BeginFrameTimeProfiling()
{
    Statistics::startFrameTimePoint = std::chrono::high_resolution_clock::now();
}

void Statistics::EndFrameTimeProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startFrameTimePoint ).count();
    Statistics::frameTimeMS = static_cast< float >(tDiff);
}

void Statistics::IncFenceCalls()
{
    ++Statistics::fenceCalls;
}

void Statistics::IncAllocCalls()
{
    ++Statistics::allocCalls;
}

int Statistics::GetAllocCalls()
{
    return Statistics::allocCalls;
}

void Statistics::IncCreateConstantBufferCalls()
{
    ++Statistics::createConstantBufferCalls;
}

int Statistics::GetCreateConstantBufferCalls()
{
    return Statistics::createConstantBufferCalls;
}

int Statistics::GetBarrierCalls()
{
    return Statistics::barrierCalls;
}

int Statistics::GetFenceCalls()
{
    return Statistics::fenceCalls;
}

void Statistics::IncRenderTargetBinds()
{
    ++Statistics::renderTargetBinds;
}

void Statistics::IncVertexBufferBinds()
{
    ++Statistics::vertexBufferBinds;
}

void Statistics::IncTextureBinds()
{
    ++Statistics::textureBinds;
}

void Statistics::IncShaderBinds()
{
    ++Statistics::shaderBinds;
}

void Statistics::IncDrawCalls()
{
    ++Statistics::drawCalls;
}

float Statistics::GetFrameTimeMS()
{
    return Statistics::frameTimeMS;
}

float Statistics::GetShadowMapTimeMS()
{
    return Statistics::shadowMapTimeMS;
}

float Statistics::GetDepthNormalsTimeMS()
{
    return Statistics::depthNormalsTimeMS;
}

int Statistics::GetDrawCalls()
{
    return Statistics::drawCalls;
}

int Statistics::GetTextureBinds()
{
    return Statistics::textureBinds;
}

int Statistics::GetRenderTargetBinds()
{
    return Statistics::renderTargetBinds;
}

int Statistics::GetVertexBufferBinds()
{
    return Statistics::vertexBufferBinds;
}

int Statistics::GetShaderBinds()
{
    return Statistics::shaderBinds;
}

void Statistics::IncBarrierCalls()
{
    ++Statistics::barrierCalls;
}

void Statistics::ResetFrameStatistics()
{
    drawCalls = 0;
    barrierCalls = 0;
    fenceCalls = 0;
    textureBinds = 0;
    shaderBinds = 0;
    vertexBufferBinds = 0;
    renderTargetBinds = 0;
    createConstantBufferCalls = 0;
    allocCalls = 0;
    triangleCount = 0;

    startFrameTimePoint = std::chrono::high_resolution_clock::now();
}

void UpdateFrameTiming()
{
    Statistics::EndFrameTimeProfiling();
}
