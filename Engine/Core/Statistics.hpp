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
    float GetPrimaryPassTimeGpuMS();

    float GetBloomCpuTimeMS();
    float GetBloomGpuTimeMS();
    
    void BeginFrameTimeProfiling();
    void EndFrameTimeProfiling();

    void BeginLightUpdateProfiling();
    void EndLightUpdateProfiling();
    float GetLightUpdateTimeMS();

    void BeginSceneAABB();
    void EndSceneAABB();
    
    void BeginPresentTimeProfiling();
    void EndPresentTimeProfiling();
    float GetPresentTimeMS();

    void BeginWaitForPreviousFrameProfiling();
    void EndWaitForPreviousFrameProfiling();
    float GetWaitForPreviousFrameProfiling();

    float GetFrustumCullTimeMS();
    void IncFrustumCullTime( float ms );
    void IncQueueWaitTime( float ms );
    float GetQueueWaitTimeMS();
    void SetBloomTime( float cpuMs, float gpuMs );
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
    void IncQueueSubmitCalls();
    int GetQueueSubmitCalls();
    void SetDepthNormalsGpuTime( float timeMS );
    void SetShadowMapGpuTime( float timeMS );
    void SetLightCullerGpuTime( float timeMS );
    void SetPrimaryPassGpuTime( float timeMS );
}
