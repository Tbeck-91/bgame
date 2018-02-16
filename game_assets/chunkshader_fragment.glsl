#version 330 core

uniform sampler2DArray textureArray;
uniform vec3 camera_position;

in vec3 tex_pos;
in vec3 world_pos;
in vec3 base_normal;
in mat3 TBN;

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gAlbedo;
layout (location = 3) out vec3 gAmbientOcclusion;

out vec4 FragColor;

const float height_scale = 0.1;

vec2 ParallaxMapping(float height, vec2 texCoords, vec3 viewDir) {
    vec2 p = viewDir.xy / viewDir.z * (height * height_scale);
    return texCoords.xy - p; 
}

void main() {
    vec3 view_tangent = world_pos * TBN;
    vec3 camera_tangent = camera_position * TBN;
    vec3 tangent_dir = normalize(view_tangent - camera_tangent);
    vec2 TexCoords = ParallaxMapping(texture(textureArray, vec3(tex_pos.x, tex_pos.y, tex_pos.z+2)).r, tex_pos.xy, tangent_dir);

    vec3 base_color = texture(textureArray, vec3(TexCoords.x, TexCoords.y, tex_pos.z)).rgb;
    gAlbedo = base_color;
    gPosition = vec3(world_pos);

    vec3 norm = texture(textureArray, vec3(TexCoords.x, TexCoords.y, tex_pos.z+1)).rgb *0.5 + 0.5;
    norm = normalize(norm);
    norm = normalize(TBN * norm);
    gNormal = norm;

    gAmbientOcclusion.r = texture(textureArray, vec3(TexCoords.x, TexCoords.y, tex_pos.z+2)).r;
    gAmbientOcclusion.g = texture(textureArray, vec3(TexCoords.x, TexCoords.y, tex_pos.z+2)).g;
    gAmbientOcclusion.b = texture(textureArray, vec3(TexCoords.x, TexCoords.y, tex_pos.z+2)).b;    
}