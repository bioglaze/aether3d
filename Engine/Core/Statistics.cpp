#include "Statistics.hpp"
#include "GfxDevice.hpp"
#include <chrono>

namespace Statistics
{
    int drawCalls = 0;
    int barrierCalls = 0;
    int fenceCalls = 0;
    int shaderBinds = 0;
    int renderTargetBinds = 0;
    int createConstantBufferCalls = 0;
    int allocCalls = 0;
    int totalAllocCalls = 0;
    int triangleCount = 0;
    int psoBindCount = 0;
    float depthNormalsTimeMS = 0;
    float depthNormalsTimeGpuMS = 0;
    float shadowMapTimeMS = 0;
    float shadowMapTimeGpuMS = 0;
    float frameTimeMS = 0;
    float presentTimeMS = 0;
    float sceneAABBTimeMS = 0;
    float lightCullerTimeGpuMS = 0;
    std::chrono::time_point< std::chrono::high_resolution_clock > startFrameTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startShadowMapTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startDepthNormalsTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startPresentTimePoint;
    std::chrono::time_point< std::chrono::high_resolution_clock > startSceneAABBPoint;
}

void Statistics::IncTriangleCount( int triangles )
{
    triangleCount += triangles;
}

int Statistics::GetTriangleCount()
{
    return triangleCount;
}

float Statistics::GetPresentTimeMS()
{
    return presentTimeMS;
}

void Statistics::SetDepthNormalsGpuTime( float timeMS )
{
    depthNormalsTimeGpuMS = timeMS;
}

void Statistics::SetShadowMapGpuTime( float timeMS )
{
    shadowMapTimeGpuMS = timeMS;
}

void Statistics::SetLightCullerTimeGpuMS( float timeMS )
{
    lightCullerTimeGpuMS = timeMS;
}

void Statistics::BeginPresentTimeProfiling()
{
    Statistics::startPresentTimePoint = std::chrono::high_resolution_clock::now();
}

void Statistics::EndPresentTimeProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startPresentTimePoint ).count();
    Statistics::presentTimeMS = static_cast< float >(tDiff);
}

void Statistics::BeginLightCullerProfiling()
{
    ae3d::GfxDevice::BeginLightCullerGpuQuery();
}

void Statistics::EndLightCullerProfiling()
{
    ae3d::GfxDevice::EndLightCullerGpuQuery();
}

void Statistics::BeginShadowMapProfiling()
{
    Statistics::startShadowMapTimePoint = std::chrono::high_resolution_clock::now();
    ae3d::GfxDevice::BeginShadowMapGpuQuery();
}

void Statistics::EndShadowMapProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startShadowMapTimePoint ).count();
    Statistics::shadowMapTimeMS = static_cast< float >(tDiff);
    ae3d::GfxDevice::EndShadowMapGpuQuery();
}

void Statistics::BeginDepthNormalsProfiling()
{
    Statistics::startDepthNormalsTimePoint = std::chrono::high_resolution_clock::now();
    ae3d::GfxDevice::BeginDepthNormalsGpuQuery();
}

void Statistics::EndDepthNormalsProfiling()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startDepthNormalsTimePoint ).count();
    Statistics::depthNormalsTimeMS = static_cast< float >(tDiff);
    ae3d::GfxDevice::EndDepthNormalsGpuQuery();
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

void Statistics::IncPSOBindCalls()
{
    ++Statistics::psoBindCount;
}

int Statistics::GetPSOBindCalls()
{
    return Statistics::psoBindCount;
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

void Statistics::IncTotalAllocCalls()
{
    ++Statistics::totalAllocCalls;
}

int Statistics::GetTotalAllocCalls()
{
    return Statistics::totalAllocCalls;
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

float Statistics::GetDepthNormalsTimeGpuMS()
{
    return Statistics::depthNormalsTimeGpuMS;
}

float Statistics::GetShadowMapTimeGpuMS()
{
    return Statistics::shadowMapTimeGpuMS;
}

int Statistics::GetDrawCalls()
{
    return Statistics::drawCalls;
}

int Statistics::GetRenderTargetBinds()
{
    return Statistics::renderTargetBinds;
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
    shaderBinds = 0;
    renderTargetBinds = 0;
    createConstantBufferCalls = 0;
    allocCalls = 0;
    triangleCount = 0;
    psoBindCount = 0;

    startFrameTimePoint = std::chrono::high_resolution_clock::now();
}

float Statistics::GetLightCullerTimeGpuMS()
{
    return lightCullerTimeGpuMS;
}

float Statistics::GetSceneAABBTimeMS()
{
    return sceneAABBTimeMS;
}

void Statistics::BeginSceneAABB()
{
    Statistics::startSceneAABBPoint = std::chrono::high_resolution_clock::now();
}

void Statistics::EndSceneAABB()
{
    auto tEnd = std::chrono::high_resolution_clock::now();
    auto tDiff = std::chrono::duration<double, std::milli>( tEnd - Statistics::startSceneAABBPoint ).count();
    Statistics::sceneAABBTimeMS = static_cast< float >(tDiff);
}

void UpdateFrameTiming()
{
    Statistics::EndFrameTimeProfiling();
}
