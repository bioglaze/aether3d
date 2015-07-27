#version 330 core

uniform mat4 _ModelViewProjectionMatrix;
uniform vec4 textureMap_ST;

layout (location = 0) in vec4 aPosition;
layout (location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

void main()
{
    gl_Position = _ModelViewProjectionMatrix * aPosition;
    vTexCoord = aTexCoord * textureMap_ST.xy + textureMap_ST.zw;
}
