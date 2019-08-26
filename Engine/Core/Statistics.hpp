#pragma once

namespace Statistics
{
    void BeginLightCullerProfiling();
    void EndLightCullerProfiling();

    void BeginShadowMapProfiling();
    void EndShadowMapProfiling();

    void BeginDepthNormalsProfiling();
    void EndDepthNormalsProfiling();

    float GetFrameTimeMS();
    float GetShadowMapTimeMS();
    float GetShadowMapTimeGpuMS();
    float GetDepthNormalsTimeMS();
    float GetDepthNormalsTimeGpuMS();
    float GetSceneAABBTimeMS();
    float GetLightCullerTimeGpuMS();

    void BeginFrameTimeProfiling();
    void EndFrameTimeProfiling();

    void BeginSceneAABB();
    void EndSceneAABB();
    
    void BeginPresentTimeProfiling();
    void EndPresentTimeProfiling();
    float GetPresentTimeMS();
    
    void IncTriangleCount( int triangles );
    int GetTriangleCount();
    void IncCreateConstantBufferCalls();
    int GetCreateConstantBufferCalls();
    void IncDrawCalls();
    int GetDrawCalls();
    void IncRenderTargetBinds();
    int GetRenderTargetBinds();
    void ResetFrameStatistics();
    void IncShaderBinds();
    int GetShaderBinds();
    void IncBarrierCalls();
    int GetBarrierCalls();
    void IncFenceCalls();
    int GetFenceCalls();
    void IncAllocCalls();
    int GetAllocCalls();
    void IncTotalAllocCalls();
    int GetTotalAllocCalls();
    void IncPSOBindCalls();
    int GetPSOBindCalls();
    void SetDepthNormalsGpuTime( float timeMS );
    void SetShadowMapGpuTime( float timeMS );
    void SetLightCullerTimeGpuMS( float timeMS );
}
