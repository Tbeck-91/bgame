#version 120

uniform sampler2D albedo_tex;
uniform sampler2D position_tex;
uniform sampler2D normal_tex;

uniform vec3 cameraPosition;
uniform vec3 ambient_color;
uniform sampler2D light_color;
uniform sampler2D light_position;

vec3 light_dir;

vec3 diffuse_light(vec3 light_pos, vec3 surface_pos, vec3 surface_normal, vec3 color) {
    vec3 result = color;
    result.xyz *= dot(light_dir, surface_normal);
    return result;
}

vec3 specular_light(vec3 light_pos, vec3 surface_pos, vec3 surface_normal, vec3 color) {
    float specular_strength = 0.5;
    float shininess = 8;

    vec3 view_dir = normalize(cameraPosition - surface_pos);
    vec3 reflect_dir = reflect(-light_dir, surface_normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), shininess);
    vec3 result = specular_strength * spec * color;
    return result;
}

void main() {
    vec4 base_color = texture2D( albedo_tex, gl_TexCoord[0].xy );
    vec4 position = texture2D( position_tex, gl_TexCoord[0].xy ) * 255.0;
    vec4 normal = texture2D( normal_tex, gl_TexCoord[0].xy );
    vec3 sun_moon_position = (texture2D( light_position, gl_TexCoord[0].xy ).rgb) * 255.0;
    vec3 sun_moon_color = texture2D( light_color, gl_TexCoord[0].xy ).rgb;

    normal = normalize(normal);
    light_dir = normalize(sun_moon_position - position.xyz);

    if (sun_moon_color.r == 0.0 && sun_moon_color.g == 0.0 && sun_moon_color.b == 0.0) {
        base_color.xyz *= ambient_color;
    } else {
        base_color.xyz *= (diffuse_light(sun_moon_position, position.xyz, normal.xyz, sun_moon_color) +
            specular_light(sun_moon_position, position.xyz, normal.xyz, sun_moon_color) +
            (base_color.xyz * ambient_color));
        //base_color.xyz = position.xyz;
    }

    gl_FragColor = base_color;
}