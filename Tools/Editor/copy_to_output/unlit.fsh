#version 330 core

uniform sampler2D textureMap;
uniform vec4 tint;

in vec2 vTexCoord;

out vec4 fragColor;

void main()
{
    fragColor = tint * texture( textureMap, vTexCoord );
}

