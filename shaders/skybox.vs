#version 330 core
layout(location=0) in vec3 position;

out vec3 direction;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    direction = position;
    vec4 clip = projection * view * vec4(position, 1.0);
    gl_Position = clip.xyww;
}
