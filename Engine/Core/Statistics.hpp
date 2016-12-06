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
    float GetDepthNormalsTimeMS();

    void BeginFrameTimeProfiling();
    void EndFrameTimeProfiling();

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
}

#endif
