#include "Renderer.hpp"

ae3d::Renderer renderer;

void ae3d::BuiltinShaders::Load()
{
    const char* spriteVertexSource = R"(
    #version 410 core
    
    layout (location = 0) in vec3 aPosition;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec4 aColor;

    uniform mat4 _ProjectionModelMatrix;

    out vec2 vTexCoord;
    out vec4 vColor;
    
    void main()
    {
        gl_Position = _ProjectionModelMatrix * vec4( aPosition.xyz, 1.0 );
        vTexCoord = aTexCoord;
        vColor = aColor;
    })";
    
    const char* spriteFragmentSource = R"(
    #version 410 core
    
    in vec2 vTexCoord;
    in vec4 vColor;
    out vec4 fragColor;
    
    uniform sampler2D textureMap;
    
    void main()
    {
        fragColor = texture( textureMap, vTexCoord ) * vColor;
    })";

    spriteRendererShader.Load( spriteVertexSource, spriteFragmentSource );

    const char* sdfVertexSource = R"(
    #version 410 core
    
    layout (location = 0) in vec3 aPosition;
    layout (location = 1) in vec2 aTexCoord;
    layout (location = 2) in vec4 aColor;
    
    uniform mat4 _ProjectionModelMatrix;
    
    out vec2 vTexCoord;
    out vec4 vColor;
    
    void main()
    {
        gl_Position = _ProjectionModelMatrix * vec4( aPosition.xyz, 1.0 );
        vTexCoord = aTexCoord;
        vColor = aColor;
    })";
    
    const char* sdfFragmentSource = R"(
    #version 410 core
    
    in vec2 vTexCoord;
    in vec4 vColor;
    out vec4 fragColor;

    uniform sampler2D textureMap;
    const float edgeDistance = 0.5;

    void main()
    {
        float distance = texture( textureMap, vTexCoord ).r;
        float edgeWidth = 0.7 * length( vec2( dFdx( distance ), dFdy( distance ) ) );
        float opacity = smoothstep( edgeDistance - edgeWidth, edgeDistance + edgeWidth, distance );
        fragColor = vec4( vColor.rgb, opacity );
    })";
    
    sdfShader.Load( sdfVertexSource, sdfFragmentSource );

    const char* skyboxVertexSource = R"(
    #version 410 core

    layout (location = 0) in vec3 aPosition;
    uniform mat4 _ModelViewProjectionMatrix;

    out vec3 vTexCoord;

    void main()
    {
        gl_Position = _ModelViewProjectionMatrix * vec4( aPosition.xyz, 1.0 );
        vTexCoord = aPosition;
    })";

    const char* skyboxFragmentSource = R"(
    #version 410 core

    in vec3 vTexCoord;
    out vec4 fragColor;

    uniform samplerCube skyMap;

    void main()
    {
        fragColor = texture( skyMap, vTexCoord );
    })";

    skyboxShader.Load( skyboxVertexSource, skyboxFragmentSource );
    
    const char* momentsVertexSource = R"(
    #version 410 core
    
    layout (location = 0) in vec3 aPosition;
    
    uniform mat4 _ModelViewProjectionMatrix;
    
    void main()
    {
        gl_Position = _ModelViewProjectionMatrix * vec4( aPosition, 1.0 );
    })";
    
    const char* momentsFragmentSource = R"(
    #version 410 core
    
    out vec4 fragColor;
    
    void main()
    {
        float linearDepth = gl_FragCoord.z;
    
        float dx = dFdx( linearDepth );
        float dy = dFdy( linearDepth );
    
        float moment1 = linearDepth;
        float moment2 = linearDepth * linearDepth + 0.25 * (dx * dx + dy * dy);
    
        fragColor = vec4( moment1, moment2, 0.0, 1.0 );
    }
    )";

    momentsShader.Load( momentsVertexSource, momentsFragmentSource );
  
    const char* depthNormalsVertexSource = R"(
    #version 410 core
    layout (location = 0) in vec4 aPosition;
    layout (location = 3) in vec3 aNormal;
    
    uniform mat4 _ModelViewProjectionMatrix;
    uniform mat4 _ModelViewMatrix;
    
    out vec3 vPosition;
    out vec3 vNormal;
    
    void main()
    {
        gl_Position = _ModelViewProjectionMatrix * aPosition;
        vPosition = (_ModelViewMatrix * aPosition).xyz;
        vNormal = mat3( _ModelViewMatrix ) * aNormal;
    }
    )";
    
    const char* depthNormalsFragmentSource = R"(
    #version 410 core
    in vec3 vPosition;
    in vec3 vNormal;
    
    out vec4 outDepthNormal;
    
    void main()
    {
        float linearDepth = vPosition.z;
        
        outDepthNormal = vec4( linearDepth, normalize( vNormal ) );
    }
    )";

    depthNormalsShader.Load( depthNormalsVertexSource, depthNormalsFragmentSource );
}
