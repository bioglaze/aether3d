#ifndef STATISTICS_HPP
#define STATISTICS_HPP

namespace Statistics
{
    void BeginShadowMapProfiling();
    void EndShadowMapProfiling();

    void BeginDepthNormalsProfiling();
    void EndDepthNormalsProfiling();

    float GetFrameTimeMS();
    float GetShadowMapTimeMS();
    float GetShadowMapTimeGpuMS();
    float GetDepthNormalsTimeMS();
    float GetDepthNormalsTimeGpuMS();

    void BeginFrameTimeProfiling();
    void EndFrameTimeProfiling();

    void BeginPresentTimeProfiling();
    void EndPresentTimeProfiling();
    float GetPresentTimeMS();
    
    void IncTriangleCount( int triangles );
    int GetTriangleCount();
    void IncCreateConstantBufferCalls();
    int GetCreateConstantBufferCalls();
    void IncDrawCalls();
    int GetDrawCalls();
    void IncVertexBufferBinds();
    int GetVertexBufferBinds();
    void IncTextureBinds();
    int GetTextureBinds();
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
    void SetDepthNormalsGpuTime( float theTime );
    void SetShadowMapGpuTime( float theTime );
}

#endif
