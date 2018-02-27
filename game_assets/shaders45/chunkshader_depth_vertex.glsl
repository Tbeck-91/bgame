#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 texture_info;
layout (location = 2) in vec3 aNormal;

layout (std140) uniform camera_data
{
	mat4 projection_matrix;
	mat4 view_matrix;
	mat4 proj_view_matrix;
};

void main()
{
    gl_Position = projection_matrix * view_matrix * vec4(aPos, 1.0);
}
